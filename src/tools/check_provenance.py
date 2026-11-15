#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check that the protected files (firmware/hal, firmware/loader) still match the imported hashes."""
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
bad = 0
for line in (ROOT / "provenance-hashes.txt").read_text().splitlines():
    sha, rel = line.split(maxsplit=1)
    f = ROOT / "firmware" / rel.strip()
    if not f.exists() or hashlib.sha256(f.read_bytes()).hexdigest() != sha:
        print(f"CHANGED  {rel.strip()}")
        bad += 1
print("provenance: protected files unchanged" if not bad else f"provenance: {bad} protected file(s) differ")
sys.exit(1 if bad else 0)
