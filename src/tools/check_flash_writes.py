#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Flash-write call-site audit (T0.5): the low-level flash primitives may be called only from the files and in the numbers
listed in tools/flash-write-sites.txt. Everything below hal/ and loader/ is protected separately (provenance hashes).

  tools/check_flash_writes.py            check (exit 1 on a new or extra call site)
  tools/check_flash_writes.py --print    print the current counts in the baseline format

A new writer must go through firmware/src/lgs_store.h (listed with 0 until it exists); add it to the baseline in the same
change, and review it against DOCs/06-storage-and-safety.md.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 and sys.argv[1] == "--src" else ROOT / "firmware" / "src"
PRIM = re.compile(r"\b(fl_write|fl_erase4k)\s*\(")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def counts():
    out = {}
    for f in sorted(SRC.glob("*")):
        if f.suffix not in (".c", ".h"):
            continue
        text = COMMENTS.sub("", f.read_text(errors="replace"))
        n = len(PRIM.findall(text))
        if n:
            out[f.name] = n
    return out


def baseline():
    base = {}
    for line in (ROOT / "tools" / "flash-write-sites.txt").read_text().splitlines():
        line = line.split("#")[0].strip()
        if line:
            name, n = line.split()
            base[name] = int(n)
    return base


def main():
    cur = counts()
    if "--print" in sys.argv:
        for k, v in cur.items():
            print(f"{k} {v}")
        return 0
    base = baseline()
    bad = 0
    for k, v in cur.items():
        if k not in base:
            print(f"NEW WRITER  {k}: {v} call(s) to fl_write/fl_erase4k, not in tools/flash-write-sites.txt")
            bad += 1
        elif v > base[k]:
            print(f"MORE WRITES {k}: {v} call(s), baseline {base[k]}")
            bad += 1
    print("flash writes: call sites match the baseline" if not bad else f"flash writes: {bad} violation(s)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
