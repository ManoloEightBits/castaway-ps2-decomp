// Applies the decomp's config/symbols.txt names to the Castaway Ghidra project.
// Input lines: <hexaddr> <F|D> <mangled> <display name>   (written by pkg/tools/gsync.py)
// F lines get a function (created if missing) named <display name> with the
// namespace split on "::"; D lines get a primary label.
// The kind comes from the caller because the PS2 loader marks every block executable.
// Names set by hand in Ghidra (SourceType.USER_DEFINED) are never overwritten. Nothing
// is cleared: a name that leaves symbols.txt simply stays until the next full names run.
//@category Castaway
//@author castaway decomp helper

import java.io.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.app.util.NamespaceUtils;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.*;

public class ApplySymbols extends GhidraScript {

    private List<String> splitPath(String s) {
        List<String> out = new ArrayList<>();
        int depth = 0, start = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '<') depth++;
            else if (c == '>') depth--;
            else if (depth == 0 && c == ':' && i + 1 < s.length() && s.charAt(i + 1) == ':') {
                out.add(s.substring(start, i));
                i++;
                start = i + 1;
            }
        }
        out.add(s.substring(start));
        return out;
    }

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("usage: ApplySymbols.java <symbols file>");
            return;
        }
        SymbolTable st = currentProgram.getSymbolTable();
        Namespace global = currentProgram.getGlobalNamespace();
        int functions = 0, created = 0, labels = 0, kept = 0, failed = 0;

        try (BufferedReader r = new BufferedReader(new FileReader(args[0]))) {
            String line;
            while ((line = r.readLine()) != null) {
                if (monitor.isCancelled()) break;
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] p = line.split(" ", 4);
                if (p.length < 4) continue;
                String hex = p[0].startsWith("0x") ? p[0].substring(2) : p[0];
                Address addr = toAddr(Long.parseLong(hex, 16));
                boolean isCode = p[1].equals("F");
                String mangled = p[2];
                String display = p[3].trim();

                MemoryBlock block = currentProgram.getMemory().getBlock(addr);
                if (block == null) { println("outside memory: " + line); failed++; continue; }

                List<String> parts = splitPath(display);
                String name = parts.remove(parts.size() - 1).replace(' ', '_');
                Namespace ns = global;
                if (!parts.isEmpty()) {
                    try {
                        ns = NamespaceUtils.createNamespaceHierarchy(String.join("::", parts), global,
                                currentProgram, SourceType.IMPORTED);
                    } catch (Exception e) {
                        ns = global;
                        name = display.replace("::", "__").replace(' ', '_');
                    }
                }

                if (isCode) {
                    Function fn = getFunctionAt(addr);
                    if (fn == null) {
                        disassemble(addr);
                        fn = createFunction(addr, null);
                        if (fn == null) { println("could not create function at " + addr + " " + mangled); failed++; continue; }
                        created++;
                    }
                    if (fn.getSymbol().getSource() == SourceType.USER_DEFINED) { kept++; continue; }
                    if (fn.getName(true).equals(display)) { functions++; continue; }
                    try {
                        fn.setParentNamespace(ns);
                        fn.setName(name, SourceType.IMPORTED);
                    } catch (Exception e) {
                        try {
                            fn.setParentNamespace(global);
                            fn.setName(mangled, SourceType.IMPORTED);
                        } catch (Exception e2) {
                            println("name failed at " + addr + ": " + display + " (" + e2.getMessage() + ")");
                            failed++;
                            continue;
                        }
                    }
                    String plate = getPlateComment(addr);
                    String tag = "symbols.txt: " + mangled;
                    if (plate == null) setPlateComment(addr, tag);
                    else if (!plate.contains(tag)) setPlateComment(addr, plate + "\n" + tag);
                    functions++;
                } else {
                    Symbol primary = st.getPrimarySymbol(addr);
                    if (primary != null && primary.getSource() == SourceType.USER_DEFINED) { kept++; continue; }
                    if (primary != null && primary.getName(true).equals(display)) { labels++; continue; }
                    try {
                        Symbol s = st.createLabel(addr, name, ns, SourceType.IMPORTED);
                        s.setPrimary();
                    } catch (Exception e) {
                        try {
                            Symbol s = st.createLabel(addr, mangled, global, SourceType.IMPORTED);
                            s.setPrimary();
                        } catch (Exception e2) {
                            println("label failed at " + addr + ": " + display + " (" + e2.getMessage() + ")");
                            failed++;
                            continue;
                        }
                    }
                    labels++;
                }
            }
        }
        println(String.format("ApplySymbols: functions %d (new %d), labels %d, kept hand names %d, failed %d",
            functions, created, labels, kept, failed));
    }
}
