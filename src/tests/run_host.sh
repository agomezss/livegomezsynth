#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run the application shell on this computer against the fake FM-1 (tests/host.c): sound -> build/host/lgs.wav, screens -> PNG.
#   tests/run_host.sh [script]       (a Linux compiler is needed: on macOS it runs in the lgs-hosttest Docker image,
#                                     see DOCs/notes/T0.1-build-notes.md; the firmware uses ELF sections)
set -e
cd "$(dirname "$0")/.."
mkdir -p build/host
BUILD='cc -O1 -Wall -Wno-unused-function -DOBV_HOST -DBOARD_SMK37=1 -DOBV_VOICE_HW=1 -DAPP_VER="\"dev\"" -DOBV_DATA_FAMILY="\"LIVEGMZ\"" -Ifirmware/src -Ifirmware/gen -Ifirmware/hal -o build/host/lgs-host firmware/src/lgs_app.c && build/host/lgs-host build/host/lgs '"$1"
if [ "$(uname -s)" = Linux ]; then
    sh -c "$BUILD"
else
    docker run --rm --platform linux/amd64 -v "$PWD:/work" -w /work lgs-hosttest sh -c "$BUILD"
fi
python3 - <<'PY'
from pathlib import Path
try:
    from PIL import Image
except ImportError:
    raise SystemExit("PIL missing: screens stay as .ppm (pip install Pillow)")
for p in sorted(Path("build/host").glob("lgs-*.ppm")):
    Image.open(p).resize((480, 480), Image.NEAREST).save(p.with_suffix(".png"))
    p.unlink()
    print("screen ->", p.with_suffix(".png"))
PY
