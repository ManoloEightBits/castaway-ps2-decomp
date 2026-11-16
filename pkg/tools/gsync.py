#!/usr/bin/env python3
"""Push config/symbols.txt names into the Ghidra project, so the decompiler shows them.

    python pkg/tools/gsync.py [--dry-run]

Reads config/symbols.txt, turns each GCC 2.95 mangled name into Class::Name for display,
borrows the template arguments this demangler does not reconstruct from the names CSV,
writes <out>/_symbols_ghidra.txt and runs pkg/ApplySymbols.java headless on the project.
symbols.txt wins over the CSV because its names are proven by the byte match, and the
CSV's propagated-tier names go stale. Names set by hand in Ghidra are never overwritten.
--dry-run writes the list and prints a sample without starting Ghidra. Rerun after each
round of matching.

The project embeds the game executable, so after a run refresh your own backup of it and
never commit it. Settings are the same environment variables as gdec.py:

    CASTAWAY_GHIDRA           Ghidra install folder (falls back to GHIDRA_INSTALL_DIR).
    CASTAWAY_GHIDRA_PROJECTS  folder holding the Ghidra project. Required.
    CASTAWAY_GHIDRA_PROJECT   project name; default Castaway.
    CASTAWAY_GHIDRA_PROGRAM   program name inside the project; default SLUS_216.64.
    CASTAWAY_GHIDRA_OUT       where the list and log go; default <projects>/castaway_drafts.
    CASTAWAY_KIT              the repo root; default is where this script lives.
"""
import csv
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.environ.get("CASTAWAY_KIT") or os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(KIT, "pkg")  # ApplySymbols.java lives beside ApplyCastawayNames.java

# The PS2 loader marks every block executable, so Ghidra cannot tell code from data by
# permissions. The ELF section table of SLUS_216.64 can: .text.animation ends at 0x491E60
# and .data starts at 0x491E80, so an address below this is a function and above it a label.
CODE_END = 0x491E80

OPERATORS = {
    "ct": "ctor", "dt": "dtor", "as": "operator=", "eq": "operator==", "ne": "operator!=",
    "lt": "operator<", "gt": "operator>", "le": "operator<=", "ge": "operator>=",
    "pl": "operator+", "mi": "operator-", "ml": "operator*", "dv": "operator/", "md": "operator%",
    "vc": "operator[]", "cl": "operator()", "rf": "operator->", "nw": "operator new",
    "dl": "operator delete", "vn": "operator new[]", "vd": "operator delete[]",
    "pp": "operator++", "mm": "operator--", "aa": "operator&&", "oo": "operator||",
    "nt": "operator!", "co": "operator~", "ad": "operator&", "or": "operator|", "er": "operator^",
    "ls": "operator<<", "rs": "operator>>", "apl": "operator+=", "ami": "operator-=",
    "amu": "operator*=", "adv": "operator/=", "aad": "operator&=", "aor": "operator|=",
    "aer": "operator^=", "als": "operator<<=", "ars": "operator>>=",
}


def settings():
    """Ghidra paths from the environment: (headless launcher, projects dir, project, program, out dir)."""
    ghidra = os.environ.get("CASTAWAY_GHIDRA") or os.environ.get("GHIDRA_INSTALL_DIR")
    projects = os.environ.get("CASTAWAY_GHIDRA_PROJECTS")
    missing = [n for n, v in (("CASTAWAY_GHIDRA", ghidra), ("CASTAWAY_GHIDRA_PROJECTS", projects)) if not v]
    if missing:
        sys.exit("set %s (see the docstring at the top of %s)" % (" and ".join(missing), __file__))
    launcher = os.path.join(ghidra, "support", "analyzeHeadless" + (".bat" if os.name == "nt" else ""))
    if not os.path.isfile(launcher):
        sys.exit("no headless launcher at %s; CASTAWAY_GHIDRA must be the Ghidra install folder" % launcher)
    project = os.environ.get("CASTAWAY_GHIDRA_PROJECT", "Castaway")
    program = os.environ.get("CASTAWAY_GHIDRA_PROGRAM", "SLUS_216.64")
    out = os.environ.get("CASTAWAY_GHIDRA_OUT") or os.path.join(projects, "castaway_drafts")
    return launcher, projects, project, program, out


def read_class(s, i):
    """Parse one class encoding at s[i:]; return (display, new index) or (None, i)."""
    if i < len(s) and s[i] == "C":
        i += 1
    if i < len(s) and s[i] == "Q":
        m = re.match(r"Q(\d)", s[i:])
        if not m:
            return None, i
        n = int(m.group(1))
        i += m.end()
        names = []
        for _ in range(n):
            part, i = read_class(s, i)
            if part is None:
                return None, i
            names.append(part)
        return "::".join(names), i
    if i < len(s) and s[i] == "t":
        m = re.match(r"t(\d+)", s[i:])
        if not m:
            return None, i
        ln = int(m.group(1))
        start = i + m.end()
        name = s[start:start + ln]
        # template arguments follow (<count> then encoded types); not reconstructed
        return name + "<>", len(s)
    m = re.match(r"(\d+)", s[i:])
    if not m:
        return None, i
    ln = int(m.group(1))
    start = i + m.end()
    if start + ln > len(s):
        return None, i
    return s[start:start + ln], start + ln


def demangle(mangled):
    """GCC 2.95 mangled name -> Class::Name, or None when it is not a function or member."""
    s = mangled
    if s.startswith("_GLOBAL_") or s.startswith("__tf") or s.startswith("__ti"):
        return None
    if s.startswith("_vt$") or s.startswith("_vt."):
        cls, _ = read_class(s[4:], 0)
        return cls + "::vtable" if cls else None
    if s.startswith("_$_") or s.startswith("_._"):
        cls, _ = read_class(s[3:], 0)
        return "%s::~%s" % (cls, cls.split("::")[-1].split("<")[0]) if cls else None
    if "$" in s and s.startswith("_"):
        cls, i = read_class(s[1:], 0)
        if cls and s[1 + i:].startswith("$"):
            return cls + "::" + s[2 + i:]
        return None
    # member or free function: <fn>__<class><args> or <fn>__F<args>
    if s.startswith("__"):
        m = re.match(r"__([a-z]+)__(.*)", s)
        if not m:
            return None
        op, rest = m.group(1), m.group(2)
        cls, _ = read_class(rest, 0)
        if cls is None:
            return None
        base = cls.split("::")[-1].split("<")[0]
        fn = {"ct": base, "dt": "~" + base}.get(op) or OPERATORS.get(op, "__" + op)
        return cls + "::" + fn
    m = re.match(r"(.+?)__([CFQt\d].*)$", s)
    if not m:
        return None  # plain C symbol such as strlen: keep as is
    fn, rest = m.group(1), m.group(2)
    if rest.startswith("F"):
        return fn
    cls, _ = read_class(rest, 0)
    if cls is None:
        return None
    return cls + "::" + fn


def build_list():
    """Lines of '<hexaddr> <F|D> <mangled> <display>' for every address in symbols.txt."""
    csv_names = {}
    csv_path = os.path.join(KIT, "pkg", "castaway_ps2_names.csv")
    if os.path.isfile(csv_path):
        with open(csv_path, encoding="utf-8", errors="replace", newline="") as f:
            for row in csv.DictReader(f):
                try:
                    csv_names[int(row["ps2_addr"], 16)] = row["name"]
                except (KeyError, ValueError):
                    pass

    lines = []
    plain = 0
    with open(os.path.join(KIT, "config", "symbols.txt"), encoding="utf-8") as f:
        for line in f:
            m = re.match(r"\s*0x([0-9A-Fa-f]+)\s+(\S+)", line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            mangled = m.group(2)
            display = demangle(mangled)
            if display is None:
                if "__" in mangled or "$" in mangled:
                    continue  # compiler-internal; leave to Ghidra
                display = mangled
                plain += 1
            # symbols.txt names are proven by the byte match; the CSV only fills in the
            # template arguments this demangler does not reconstruct
            if "<>" in display and addr in csv_names and csv_names[addr].startswith(display.split("<")[0] + "<"):
                display = csv_names[addr]
            kind = "F" if addr < CODE_END else "D"
            lines.append("%08X %s %s %s" % (addr, kind, mangled, display))
    return lines, plain


def main(argv):
    dry = "--dry-run" in argv
    launcher, projects, project, program, out = settings()
    lines, plain = build_list()

    os.makedirs(out, exist_ok=True)
    listfile = os.path.join(out, "_symbols_ghidra.txt")
    with open(listfile, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("%d names (%d plain C) -> %s" % (len(lines), plain, listfile))
    if dry:
        for l in lines[:8] + lines[-8:]:
            print("  " + l)
        return

    if os.path.exists(os.path.join(projects, project + ".lock")):
        sys.exit("the %s project is open in Ghidra; close it first" % project)
    cmd = [launcher, projects, project, "-process", program, "-noanalysis",
           "-scriptPath", SCRIPTS, "-postScript", "ApplySymbols.java", listfile]
    proc = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    log = proc.stdout + proc.stderr
    logfile = os.path.join(out, "_last_sync.log")
    with open(logfile, "w") as f:
        f.write(log)
    for l in log.splitlines():
        if "ApplySymbols" in l and ("functions" in l or "failed" in l or "could not" in l):
            print(l.split("ApplySymbols.java>")[-1].strip())
    if proc.returncode != 0:
        sys.exit("headless run failed (exit %d); see %s" % (proc.returncode, logfile))


if __name__ == "__main__":
    main(sys.argv[1:])
