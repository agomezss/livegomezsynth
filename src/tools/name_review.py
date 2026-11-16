#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Name review (D-42, T4.1): every name shown by the instrument is checked against a list of trademarks, instrument models and
artist names. Names may hint at a song or a sound; they must not use a brand or a person's name.

  python3 tools/name_review.py          report; exit 1 when something is found
"""
import json, pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
DENY = """
rhodes wurlitzer wurly wurli moog minimoog prophet oberheim ob-x obxa juno jupiter korg roland yamaha casio dx7 dx-7 fm7 farfisa vox
hammond b3 leslie fender stratocaster telecaster gibson les paul marshall steinway bosendorfer kawai clavinet hohner mellotron
chamberlin linn lindrum 808 909 tr-808 tr-909 ppg fairlight synclavier emulator emu akai sequential memorymoog odyssey
cs-80 cs80 sh-101 tb-303 tb303 303 d-50 m1 jx-3p jx8p polysix mono/poly prophet-5 minilogue elka solina ensoniq sonic
santana wakeman floyd mercury queen purple zeppelin sabbath iron maiden metallica beatles stones doors kiss bonjovi
jackson madonna prince whitney bowie jagger gabriel collins lennox lauper depeche cure smiths nirvana pearl radiohead
coldplay oasis muse linkin shaman nightwish legiao paralamas titas barao capital engenheiros skank kid abelha blitz
""".split()
# whole words only; a word may stay when it is an ordinary English word here (and only with the reason written down)
ALLOW = {"purple": "a colour: Purple Pad / Purple Drv name no band", "sonic": "an adjective",
         "queen": "the owner's explicit choice (SYN Queen Str, patch sheet 2026-11-16), D-42 knowingly set aside for these two"}


def names():
    out = {}
    for p in json.load(open(root / "catalog" / "catalog.json"))["patches"]:
        out[p["name"]] = "catalog"
    for f in sorted((root / "firmware" / "src").glob("lgs_*.h")) + sorted((root / "firmware" / "gen").glob("*.h")):
        if f.name == "lgs_catalog_data.h":
            continue
        for m in re.finditer(r'\{"([A-Z0-9][A-Z0-9 .\'+-]{2,16})", (?:\{|P[FX]_)', f.read_text()):
            out.setdefault(m.group(1), f.name)
    return out


def main():
    hits = []
    for n, where in sorted(names().items()):
        words = re.findall(r"[a-z0-9/+-]+", n.lower())
        for d in DENY:
            if " " in d:
                if d in n.lower():
                    hits.append((n, d, where))
            elif d in words and d not in ALLOW:
                hits.append((n, d, where))
    for n, d, where in hits:
        print("  NAME  %-18s  matches %-10s (%s)" % (n, d, where))
    print("name review: %d names checked, %d findings" % (len(names()), len(hits)))
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
