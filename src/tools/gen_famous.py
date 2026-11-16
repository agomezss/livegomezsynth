#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The famous-timbre recipes: writes the SYN and FM6 parts of tools/famous_defs.py into the hand-kept headers (between their
FAMOUS markers). The ICONIC category is gone (D-66): catalog/catalog.json is edited by hand and its entries keep pointing at
these preset rows by index, so a recipe row is never removed or reordered. The FM2 / ORG / SP parts are spliced by the
generators of those engines (port_clipper_engines.py, gen_lgs_samples.py). Run those first, then this, then gen_catalog.py."""
import json, pathlib, re, sys
root = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / "tools"))
import famous_defs as D

src = root / "firmware" / "src"


def between(s, a, b, body):
    i, j = s.index(a) + len(a), s.index(b)
    return s[:i] + "\n" + body + "    " + s[j:] if body else s[:i] + "\n    " + s[j:]


# -- SYN: parameter sets + FACTORY_LGS rows
fl = src / "lgs_flavors.h"
s = fl.read_text()
pf, rw = "", ""
for k, (name, d) in enumerate(D.SYN):
    toks = []
    for key, v in d.items():
        toks += ["P_" + key, str(v)]
    toks.append("X")
    pf += "static const uint8_t PF_FS%d[] = {%s};\n" % (k, ", ".join(toks))
    rw += '    {"%s", PF_FS%d},\n' % (name, k)
s = between(s, "/* FAMOUS-PF BEGIN (tools/gen_famous.py) */", "/* FAMOUS-PF END */", pf)
s = between(s, "/* FAMOUS-ROWS BEGIN */", "/* FAMOUS-ROWS END */", rw)
fl.write_text(s)
# the SYN base index is NFACT(64) + position in FACTORY_LGS: count its rows
fs = fl.read_text()
tab = fs[fs.index("FACTORY_LGS[] = {"):]
tab = tab[:tab.index("/* FAMOUS-ROWS BEGIN */")]
base_syn = len(re.findall(r'\{"[^"]+", PF_', tab))

# -- FM6
f6 = src / "lgs_eng_fm6.h"
s = f6.read_text()
s = between(s, "/* FAMOUS BEGIN (tools/gen_famous.py) */", "/* FAMOUS END */", D.rows(D.FM6))
f6.write_text(s)


def before_famous(path, table_start):
    s = path.read_text()
    a = s.index(table_start)
    b = s.index("/* ICONIC (tools/famous_defs.py) */", a)
    return len(re.findall(r'^\s*\{"[^"]+", \{', s[a:b], re.M))


base = {"syn": 64 + base_syn,
        "fm2": before_famous(src / "lgs_eng_fm2.h", "DIGITAL_PRESETS[] = {"),
        "org": before_famous(src / "lgs_eng_org.h", "DRAWBAR_PRESETS[] = {"),
        "sp": before_famous(root / "firmware" / "gen" / "lgs_samples.h", "SMP_PRESET_TABLE[] = {"),
        "fm6": before_famous(f6, "FM6_PRESETS[] = {")}
print("bases", base)

# The catalog entries are no longer written here (D-66): catalog/catalog.json is the source of truth, ICONIC is gone as a category,
# the recipes kept in famous_defs.py are engine presets whose catalog entries point at them by index (the `bases` above).
print("recipes:", len(D.ORDER), "(catalog.json not touched)")
