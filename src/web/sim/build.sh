#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# LiveGomezSynth in the browser: the firmware's sources + web/sim/sim.c -> WebAssembly, plus the page.
#   sh web/sim/build.sh   ->  build/sim-site/{index.html, sim.js, sim.css, worklet.js, lgs.wasm}
#   then: python3 -m http.server -d build/sim-site 8737   and open http://localhost:8737
# Tools: zig (brew install zig) and Binaryen's wasm-opt (brew install binaryen; Asyncify, see sim.c).
set -e
cd "$(dirname "$0")/../.."
ZIG="${ZIG:-zig}"
WASM_OPT="${WASM_OPT:-wasm-opt}"
command -v "$ZIG" >/dev/null || { echo "sim: no zig (brew install zig)"; exit 1; }
command -v "$WASM_OPT" >/dev/null || { echo "sim: no wasm-opt (brew install binaryen)"; exit 1; }
mkdir -p build/sim build/sim-site
nice -n 15 $ZIG cc --target=wasm32-freestanding -mcpu=generic+bulk_memory -O2 -fno-builtin -ffreestanding -nostdlib \
    -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
    -DOBV_VOICE_HW=1 -DOBV_DATA_FAMILY='"LIVEGMZ"' -DAPP_VER="\"$(tr -d ' \n' < VERSION)\"" \
    -Ifirmware/src -Ifirmware/gen -Ifirmware/hal \
    -Wl,--no-entry -Wl,--export-memory -Wl,--export-dynamic -Wl,-z,stack-size=262144 \
    -o build/sim/raw.wasm web/sim/sim.c
nice -n 15 "$WASM_OPT" build/sim/raw.wasm --enable-bulk-memory --asyncify -O2 --strip-debug --strip-producers \
    -o build/sim-site/lgs.wasm
cp web/sim/index.html web/sim/sim.js web/sim/sim.css web/sim/worklet.js build/sim-site/
echo "sim: build/sim-site ($(wc -c < build/sim-site/lgs.wasm | tr -d ' ') B wasm)"
