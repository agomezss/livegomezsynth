#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Writes catalog/patchlist.csv: every patch in the order the keyboard lists it (category, then id), for reviewing sounds one
by one. The last column is empty on purpose: mark what to drop (e.g. `x`) and hand the file back.

  python3 tools/patchlist.py
"""
import csv, json, pathlib

root = pathlib.Path(__file__).resolve().parent.parent
cat = json.load(open(root / "catalog" / "catalog.json"))
names = cat["categories"]
rows = sorted(cat["patches"], key=lambda p: (p["cat"], p["id"]))
pos = {}
with open(root / "catalog" / "patchlist.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["category", "pos", "id", "engine", "name", "shown", "drop"])
    for p in rows:
        pos[p["cat"]] = pos.get(p["cat"], 0) + 1
        shown = p["name"] if p["cat"] == 16 else p["name"][4:]
        w.writerow([names[p["cat"] - 1], pos[p["cat"]], p["id"], p["engine"], p["name"], shown, ""])
print("wrote catalog/patchlist.csv, %d patches" % len(rows))
