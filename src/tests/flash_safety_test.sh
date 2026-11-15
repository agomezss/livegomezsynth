#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Flash safety checks (T0.5): the protected files are unchanged, flash write call sites match the baseline, a NEW writer is
# refused, the flash map's compile-time asserts reject a window that crosses a protected range, and the allow-list never
# touches the head, BTIF/USRTRIM, stock USR or key_mac. Needs only a C compiler and python3.
set -e
cd "$(dirname "$0")/.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
python3 tools/check_provenance.py
python3 tools/check_flash_writes.py
cc -O1 -DBOARD_SMK37=1 -Ifirmware/hal -o "$T/smk_flash_test" tests/smk_flash_test.c && "$T/smk_flash_test" | tail -1
# 0. the first start over SMKSYNTH's bank (owner tag takeover), power cut at every step, nothing outside our sectors
cc -O1 -Wall -DBOARD_SMK37=1 -Ifirmware/hal -Ifirmware/src -o "$T/voice_takeover_test" tests/voice_takeover_test.c && "$T/voice_takeover_test" | sed 's/^/  /'
# 1. a new write call site must be refused
mkdir -p "$T/src" && cp firmware/src/*.c firmware/src/*.h "$T/src/"
printf 'static void evil(void) { fl_write(0xC1000u, 0, 0); }\n' >> "$T/src/lgs_app.c"
if python3 tools/check_flash_writes.py --src "$T/src" >/dev/null; then echo "FAIL: a new writer was accepted"; exit 1; else echo "  ok    a new fl_write call site is refused"; fi
# 2. a flash map whose FL_DATA window reaches into BTIF/USRTRIM (0xC0000) must not compile
mkdir -p "$T/hal" && cp firmware/hal/*.h "$T/hal/"
python3 - "$T/hal/fm1_flash_map.h" <<'PY'
import sys, re
p = sys.argv[1]; s = open(p).read()
s2 = s.replace("#define FL_DATA_HI      0x000BB000u", "#define FL_DATA_HI      0x000C1000u")
assert s2 != s; open(p, "w").write(s2)
PY
if cc -DBOARD_SMK37=1 -I"$T/hal" -o "$T/bad" tests/smk_flash_test.c 2>/dev/null; then echo "FAIL: an unsafe flash window compiled"; exit 1; else echo "  ok    an allow-list window crossing BTIF/USRTRIM does not compile"; fi
# 3. A/B image storage: power cut at every step, damaged copies
mkdir -p build && cc -O1 -Wall -Ifirmware/src -o build/store_ab_test tests/store_ab_test.c && build/store_ab_test | sed 's/^/  /'
echo "flash safety: all checks passed"
