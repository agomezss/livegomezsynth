# LiveGomezSynth firmware (`/src`)

Project overview, cheat sheet, risks and credits: [../README.md](../README.md). Technical handoff for agents: [../AGENTS.md](../AGENTS.md).

Own firmware for the M-Vave SMK-37 Pro, focused on live performance. This is an **independent code base, not a fork**:
the repositories next to it (`SMKSYNTH/`, `smk37_open/`, `clipper-smk/`) are **references**. Code that was taken from them
is listed in [PROVENANCE.md](PROVENANCE.md) with its license; nothing here tracks or syncs with them.

License: GPL-3.0-only ([LICENSE](LICENSE)); third-party notices in [THIRD_PARTY.md](THIRD_PARTY.md).
Specs and task cards: [../DOCs/README.md](../DOCs/README.md).

## Layout

```
firmware/   app + update loader + HAL (hal/ and loader/ are imported verbatim, see PROVENANCE.md)
tools/      lgs_build.py (our build entry), build.py + fm1pkg_make.py + lz4blk.py (imported build steps), get_toolchain.sh
tests/      run_tests.sh (all gates), ui_scenarios.mjs, engines_test.mjs, store_test.mjs, store_ab_test.c, screens_dump.mjs, flash_safety_test.sh; run_host.sh + host.c (fake FM-1, Docker on macOS)
web/sim/    browser simulator (sh web/sim/build.sh; python3 -m http.server -d build/sim-site 8737; open /index.html?autostart)
```

Application: `firmware/src/lgs_app.c` includes the OberVave voice core, the Clipper engine adapter and the LiveGomezSynth model and
UI (`lgs_model.h`, `lgs_ui.h`, `lgs_store.h`, `lgs_midi.h`). There is no step sequencer; the old monolith is gone.
Catalog: `catalog/catalog.json` -> `tools/gen_catalog.py` -> `firmware/src/lgs_catalog_data.h`.

## Build (macOS or Linux)

Prerequisites (one-time, details in `DOCs/notes/T0.1-build-notes.md`):

```sh
sh tools/get_toolchain.sh                       # JieLi toolchain -> ~/.jieli/toolchain
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK ~/fw-AC79_AIoT_SDK
python3 -m venv ~/.lgs-venv && ~/.lgs-venv/bin/pip install Pillow
# macOS: Docker (Colima) must be running; the toolchain is x86-64
```

```sh
export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK
~/.lgs-venv/bin/python tools/lgs_build.py                 # build/livegomezsynth-0.1-dev-smk37.fwsc
python3 tools/check_provenance.py                         # hal/ and loader/ must still match the imported files
sh tests/flash_safety_test.sh                             # flash-safety gates (also run by lgs_build.py: provenance + write sites)
```

Release: `python3 tools/package_release.py --build` checks names, levels and the installer, then writes `release/<version>/` and
`release/latest/` (package, manifest, SHA256SUMS, notes, install guide, and `site/`: simulator at `/`, installer at `/install/`).
Catalog tools: `node tests/catalog_health.mjs [--trim|--update]` (level match, golden renders), `python3 tools/name_review.py`.

Safety: **do not install on a device before the checklist in `DOCs/06-storage-and-safety.md` (backup first, P-01).**
Current state: M1 (PATCH / PERFORM / EDIT, favorites, solo, sends, MIDI in, panel check, storage) and most of M2 (synth, drum kits,
4-op and 6-op FM, drawbar organ, samples; 349 patches) run in the simulator and in the test suites; the package builds (283 KB of the 320 KB
slot). Nothing has run on hardware yet. Tests: `sh tests/run_tests.sh` (add `--quick` to skip the package build); the simulator
is described in `web/sim/README.md`. Still open: CPU budget on the device, install guide (after the first install).
