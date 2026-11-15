#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""LiveGomezSynth build (SMK-37 Pro only): app + update loader + installable .fwsc package.

  tools/lgs_build.py [--version <src/VERSION>] [--id 20000] [--family LIVEGMZ] [--sdk PATH]

Wraps the build steps in build.py (imported from the SMKSYNTH/OberVave code base, see ../PROVENANCE.md) without its
SMKSYNTH-specific release gating. Environment: JIELI_TOOLCHAIN (JieLi Linux toolchain), AC79_SDK (JieLi AC79 SDK
checkout, tag AC79NN_SDK_V1.2.1_2023-12-13). On macOS each toolchain tool runs in a linux/amd64 Docker container.
Output: build/livegomezsynth-<version>-smk37.fwsc
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
os.environ.setdefault("LGS_APP_SRC", "lgs_app.c")   # the application unit
import build as b  # noqa: E402
import fm1pkg_make  # noqa: E402

APP_SLOT = 0x50000          # 320 KiB; the user-data region starts at 0x55000 (T0.1 notes). Revisit in T0.3 / T0.7.


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default=(Path(__file__).resolve().parent.parent / "VERSION").read_text().strip(), help="shown on the instrument (APP_VER); default: src/VERSION")
    ap.add_argument("--id", default="20000", help="3-5 digit identity suffix: 'SMK-37 Pro_<id>'")
    ap.add_argument("--family", default="LIVEGMZ", help="flash owner tag, 1..7 of A-Z 0-9 (D-01)")
    ap.add_argument("--sdk", type=Path, default=os.environ.get("AC79_SDK"), help="JieLi AC79 SDK checkout")
    a = ap.parse_args()
    if not a.sdk:
        raise SystemExit("set AC79_SDK or pass --sdk")
    if not re.fullmatch(r"[1-9][0-9]{2,4}", a.id):
        raise SystemExit("--id must be a 3 to 5 digit positive number")
    if not re.fullmatch(r"[A-Z0-9]{1,7}", a.family):
        raise SystemExit("--family: 1..7 of A-Z 0-9")

    b.BOARD = "smk37"
    fm1pkg_make.set_board("smk37")
    b.APP_SLOT = fm1pkg_make.APP_SLOT = APP_SLOT
    b.PRODUCT = f"{b.MODELS['pro']}_{a.id}"
    if len(b.PRODUCT) > 20:
        raise SystemExit("identity exceeds the 20-marker package identity")
    for chk in ("check_provenance.py", "check_flash_writes.py"):   # flash-safety gates (DOCs/06): a failure stops the build
        if subprocess.run([sys.executable, str(Path(__file__).resolve().parent / chk)]).returncode:
            raise SystemExit(f"build: {chk} failed")
    b.VERSION = a.version
    b.VOICE_HARDWARE = True     # the SMKSYNTH 1.2.0 product profile: isolated store, bounded app file
    b.BOARD_DEFS["smk37"] = [*b.BOARD_DEFS["smk37"], "-DOBV_VOICE_HW=1"]
    os.environ["OBV_DATA_FAMILY"] = a.family

    fm1pkg_make.SDK = a.sdk
    for rel, sha in b.SDK_SHA256.items():
        if hashlib.sha256(fm1pkg_make.sdk_file(rel)).hexdigest() != sha:
            print(f"warning: SDK {rel} differs from AC79NN_SDK_V1.2.1; use the tested SDK tag")
    b.OUT.mkdir(parents=True, exist_ok=True)
    ota = b.build_loader()
    img, syms, dis, rt = b.build_app()
    errors, notes = b.check(img, syms, dis, rt)
    hal_err = b.mmio_check()
    errors += hal_err
    if not hal_err:
        notes.append("register access: hal/ only (src/, loader/ clean)")
    for n in notes:
        print("  ok   ", n)
    for e in errors:
        print("  FAIL ", e)
    if errors:
        raise SystemExit("build: checks failed")
    flash = fm1pkg_make.flash_image(img, fm1pkg_make.KEY)
    if flash[0x55000:0x9D000] != b"\xff" * 0x48000:
        raise SystemExit("user-data region overlaps declared boot files")
    pkg = fm1pkg_make.ufw(flash, ota, b.PRODUCT)
    name = f"livegomezsynth-{a.version}-smk37.fwsc"
    (b.OUT / name).write_bytes(pkg)
    print(f"app      {b.OUT / 'obervave.bin'}  {len(img)} B of {APP_SLOT}")
    print(f"loader   {b.LDR / 'ota.bin'}  {len(ota)} B")
    print(f"package  {b.OUT / name}  {len(pkg)} B, identity {b.PRODUCT}, family {a.family}")
    print(f"sha256   {hashlib.sha256(pkg).hexdigest()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
