// Writes Ghidra's decompiled C for a list of PS2 addresses to files, one per function.
// Run it through pkg/tools/gdec.py, which resolves names and picks the paths, or by hand:
//   analyzeHeadless <projects dir> <project> -process SLUS_216.64 -noanalysis -readOnly
//       -scriptPath <this folder> -postScript DecompileFunctions.java <addr list> <out dir>
// The address list has one hex address per line (0x prefix optional; a second word is
// ignored, so a symbols.txt-style "name 0xADDR" or "0xADDR name" both work). Lines
// starting with # are skipped. Output files are <addr>_<name>.c under <out dir>, each
// with a header listing the function's size and its callers, which is the evidence a
// name needs. The output is derived from the game executable: keep it out of the repo.
//@category Castaway
//@author castaway decomp helper

import java.io.*;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class DecompileFunctions extends GhidraScript {

    private static String sanitize(String s) {
        StringBuilder b = new StringBuilder();
        for (char c : s.toCharArray()) {
            b.append(Character.isLetterOrDigit(c) || c == '_' ? c : '_');
        }
        String out = b.toString();
        return out.length() > 60 ? out.substring(0, 60) : out;
    }

    private Address parseAddr(String word) {
        String w = word.trim();
        if (w.startsWith("0x") || w.startsWith("0X")) w = w.substring(2);
        return toAddr(Long.parseLong(w, 16));
    }

    private String callersOf(Function f) {
        ReferenceManager rm = currentProgram.getReferenceManager();
        TreeMap<String, Integer> callers = new TreeMap<>();
        int data = 0;
        for (Reference r : rm.getReferencesTo(f.getEntryPoint())) {
            if (r.getReferenceType().isCall()) {
                Function c = getFunctionContaining(r.getFromAddress());
                String key = c == null ? "?" : c.getName(true) + " @ " + c.getEntryPoint();
                callers.merge(key, 1, Integer::sum);
            } else if (r.getReferenceType().isData()) {
                data++; // a vtable slot or a function-pointer table
            }
        }
        StringBuilder b = new StringBuilder();
        b.append("// callers: ").append(callers.isEmpty() ? "none found" : "").append('\n');
        for (Map.Entry<String, Integer> e : callers.entrySet()) {
            b.append("//   ").append(e.getKey());
            if (e.getValue() > 1) b.append(" (x").append(e.getValue()).append(')');
            b.append('\n');
        }
        if (data > 0) b.append("// data references (vtable slots etc.): ").append(data).append('\n');
        return b.toString();
    }

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("usage: DecompileFunctions.java <address list file> <output dir>");
            return;
        }
        File list = new File(args[0]);
        File outDir = new File(args[1]);
        outDir.mkdirs();

        List<Address> addrs = new ArrayList<>();
        try (BufferedReader r = new BufferedReader(new FileReader(list))) {
            String line;
            while ((line = r.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] words = line.split("\\s+");
                String hex = null;
                for (String w : words) {
                    String t = w.startsWith("0x") || w.startsWith("0X") ? w.substring(2) : w;
                    if (t.matches("[0-9A-Fa-f]{5,8}")) { hex = w; break; }
                }
                if (hex == null) { printerr("no address in line: " + line); continue; }
                addrs.add(parseAddr(hex));
            }
        }

        DecompInterface ifc = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        opts.grabFromProgram(currentProgram);
        ifc.setOptions(opts);
        ifc.toggleCCode(true);
        ifc.setSimplificationStyle("decompile");
        if (!ifc.openProgram(currentProgram)) {
            printerr("decompiler failed to open: " + ifc.getLastMessage());
            return;
        }

        try {
            for (Address a : addrs) {
                Function f = getFunctionAt(a);
                String note = "";
                if (f == null) {
                    f = getFunctionContaining(a);
                    if (f != null) note = "// NOTE: " + a + " is inside this function, not its start\n";
                }
                if (f == null) {
                    println("DECOMP " + a + " NOFUNC");
                    continue;
                }
                String name = f.getName(true);
                File out = new File(outDir, String.format("%08x_%s.c", a.getOffset(), sanitize(name)));

                DecompileResults res = ifc.decompileFunction(f, 90, monitor);
                try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
                    w.print("// " + name + " @ " + f.getEntryPoint());
                    w.println(", " + f.getBody().getNumAddresses() + " bytes");
                    w.print(note);
                    w.print(callersOf(f));
                    w.println("// Ghidra decompiler output; a draft of the logic, not matched source.");
                    w.println();
                    if (res.decompileCompleted()) {
                        w.print(res.getDecompiledFunction().getC());
                        println("DECOMP " + a + " OK " + out.getPath());
                    } else {
                        w.println("// decompile failed: " + res.getErrorMessage());
                        println("DECOMP " + a + " FAIL " + out.getPath());
                    }
                }
            }
        } finally {
            ifc.dispose();
        }
    }
}
