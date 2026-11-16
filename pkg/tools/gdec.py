#!/usr/bin/env python3
"""Ghidra's decompiled C for PS2 functions, as a first draft of a function's logic.

    python pkg/tools/gdec.py 0x20F258 GetTileWidth__8cXObject [--force] [--quiet]

Each argument is a hex address or a substring of a name; names are resolved through
config/symbols.txt and pkg/castaway_ps2_names.csv. Ghidra runs headless on the project
with analysis off and the project read-only, so nothing in it changes. One file per
function lands in the output folder and is printed; its header lists the size and the
callers, which is the evidence a name needs. A function already in the output folder is
printed without rerunning Ghidra; --force redoes it, --quiet prints only the paths.
Headless start-up costs ten to twenty seconds however many addresses are asked for, so
batch them.

The decompiler recovers logic, not register ranks, so a draft is a reading aid for the
matching loop and never source to commit. The output is derived from the game executable
and stays outside the repo: tools/check_repo.py refuses the files by their header.

Settings, all environment variables:

    CASTAWAY_GHIDRA           Ghidra install folder (the one with support/analyzeHeadless).
                              Falls back to GHIDRA_INSTALL_DIR, the name Ghidra's own
                              extension builds use.
    CASTAWAY_GHIDRA_PROJECTS  folder holding the Ghidra project. Required.
    CASTAWAY_GHIDRA_PROJECT   project name; default Castaway.
    CASTAWAY_GHIDRA_PROGRAM   program name inside the project; default SLUS_216.64.
    CASTAWAY_GHIDRA_OUT       output folder; default <projects>/castaway_drafts, which is
                              beside the project and so already outside the repo.
    CASTAWAY_KIT              the repo root; default is where this script lives.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.environ.get("CASTAWAY_KIT") or os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(KIT, "pkg")  # DecompileFunctions.java lives beside ApplyCastawayNames.java
HEX = re.compile(r"0x([0-9A-Fa-f]{5,8})\b|\b([0-9A-Fa-f]{8})\b")


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


def check_not_open(projects, project):
    if os.path.exists(os.path.join(projects, project + ".lock")):
        sys.exit("the %s project is open in Ghidra; close it (or File > Close Project) first" % project)


def resolve(query):
    """Return (address, label) for a hex address or a name substring."""
    q = query.strip()
    m = re.fullmatch(r"(?:0x)?([0-9A-Fa-f]{5,8})", q)
    if m:
        return int(m.group(1), 16), None
    hits = {}
    for rel in ("config/symbols.txt", "pkg/castaway_ps2_names.csv"):
        path = os.path.join(KIT, rel)
        if not os.path.isfile(path):
            continue
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                if q in line and not line.startswith("#"):
                    m = HEX.search(line)
                    if m:
                        hits.setdefault(int(m.group(1) or m.group(2), 16), line.strip()[:100])
    if len(hits) == 1:
        (addr, label), = hits.items()
        return addr, label
    if not hits:
        sys.exit("no address found for %r (looked in %s)" % (query, KIT))
    print("%r is ambiguous:" % query, file=sys.stderr)
    for addr, label in sorted(hits.items()):
        print("  0x%08X  %s" % (addr, label), file=sys.stderr)
    sys.exit(2)


def existing(out, addr):
    prefix = "%08x_" % addr
    for name in os.listdir(out) if os.path.isdir(out) else []:
        if name.startswith(prefix):
            return os.path.join(out, name)
    return None


def main(argv):
    force = "--force" in argv
    quiet = "--quiet" in argv
    queries = [a for a in argv if not a.startswith("--")]
    if not queries:
        sys.exit(__doc__)
    launcher, projects, project, program, out = settings()
    wanted = [resolve(q) for q in queries]

    todo = [a for a, _ in wanted if force or not existing(out, a)]
    if todo:
        check_not_open(projects, project)
        os.makedirs(out, exist_ok=True)
        listfile = os.path.join(out, "_request.txt")
        with open(listfile, "w") as f:
            for a in todo:
                f.write("0x%08X\n" % a)
        cmd = [launcher, projects, project, "-process", program, "-noanalysis", "-readOnly",
               "-scriptPath", SCRIPTS, "-postScript", "DecompileFunctions.java", listfile, out]
        proc = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
        log = proc.stdout + proc.stderr
        with open(os.path.join(out, "_last_run.log"), "w") as f:
            f.write(log)
        status = re.findall(r"DECOMP (\S+) (\w+)", log)
        if proc.returncode != 0 or not status:
            tail = "\n".join(log.splitlines()[-25:])
            sys.exit("headless run failed (exit %d); log tail:\n%s" % (proc.returncode, tail))
        for a, s in status:
            if s != "OK":
                print("%s: %s" % (a, s), file=sys.stderr)

    for addr, label in wanted:
        path = existing(out, addr)
        if not path:
            print("0x%08X: no output" % addr, file=sys.stderr)
            continue
        if quiet:
            print(path)
            continue
        print("===== " + path)
        with open(path, encoding="utf-8", errors="replace") as f:
            sys.stdout.write(f.read())
        print()


if __name__ == "__main__":
    main(sys.argv[1:])
