#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Everything that must be green before a change is accepted (G0): simulator build, firmware scenarios, screens,
# storage, flash safety, and the device package build.   sh tests/run_tests.sh [--quick]  (--quick skips the package build)
set -e
cd "$(dirname "$0")/.."
sh web/sim/build.sh >/dev/null
echo "== UI scenarios";   node tests/ui_scenarios.mjs | grep -v "^  ok" || true
node tests/ui_scenarios.mjs >/dev/null
echo "== smoke";          node tests/sim_wasm_test.mjs >/dev/null && echo "  ok"
echo "== engines (every catalog patch)"; node tests/engines_test.mjs | grep -v "^  ok" || true
node tests/engines_test.mjs >/dev/null
echo "== catalog levels (peak, level match, golden renders)"; node tests/catalog_health.mjs | grep -v "^  ok" || true
node tests/catalog_health.mjs >/dev/null
echo "== fuzz (random panel and MIDI input)"; node tests/fuzz.mjs 3 2000 | grep -v "^  ok" || true
node tests/fuzz.mjs 3 2000 >/dev/null
echo "== names and installer"; python3 tools/name_review.py | tail -1; node tests/installer_test.mjs | grep -v "^  ok" || true
node tests/installer_test.mjs >/dev/null
python3 tools/name_review.py >/dev/null
echo "== screens";        node tests/screens_dump.mjs
echo "== storage image";  node tests/store_test.mjs >/dev/null && echo "  ok"
echo "== flash safety";   sh tests/flash_safety_test.sh | tail -1
if [ "$1" != "--quick" ]; then
    echo "== package";    python3 tools/lgs_build.py | grep -E "image |package"
fi
echo "ALL GREEN"
