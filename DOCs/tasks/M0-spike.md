# M0 - Technical spike and scaffold

Common "Read first": `DOCs/00-decisions.md`, `DOCs/06-storage-and-safety.md`.

## T0.1 Toolchain bring-up and pristine build

- Depends: -
- Read: 01 (SMKSYNTH section), `SMKSYNTH/docs/BUILD.en.md`, `SMKSYNTH/tools/get_toolchain.sh`.
- Do: obtain the JieLi pi32v2 toolchain (Linux; use Docker on macOS), build pristine `SMKSYNTH` (`BOARD=smk37 ./build.sh`),
  run its host tests (`sh tests/run_tests.sh`) and `BOARD=smk37 tests/run_host.sh`. Install Binaryen for the sim.
  Confirm: LCD columns/rows, real flash map (`fm1_flash_map.h`, `smk_store.h`), XIP size of the pristine build.
- Output: `DOCs/notes/T0.1-build-notes.md` (commands, versions, results, verified constants replacing every **VERIFY** in 01/03/06).
- Accept: pristine `.fwsc` hash equals the one in SMKSYNTH `releases/` (or documented difference); tests green.

## T0.2 Own code base under `/src` (done, no fork)

- Depends: T0.1
- Done: imported `SMKSYNTH/firmware` and the build steps into `/src` (see `src/PROVENANCE.md`); added `src/tools/lgs_build.py`
  (our build entry: family `LIVEGMZ`, identity `SMK-37 Pro_<id>`), `check_provenance.py`, LICENSE, THIRD_PARTY.md, README.
  Fidelity: building with the reference parameters reproduced the SMKSYNTH 1.2.0 package byte for byte.
  Default build: `build/livegomezsynth-0.1-dev-smk37.fwsc` (app 224,888 B).
- Left for later: the SMKSYNTH tests, the browser simulator (T0.6), brand strings on screen (splash), Elite/MKE models.

## T0.3 Budget spike (CPU, RAM, flash, dual core)

- Depends: T0.2, T0.7
- Read: 02 section 6, `clipper-smk/firmware/src/cpu1.c`, `clipper-smk/tests/hostsim.c`, `SMKSYNTH` render counters.
- Do: (1) determine whether SMKSYNTH uses the second CPU core; (2) host-benchmark SMKSYNTH ANALOG worst case and
  Clipper engines FM6, DIGITAL, SAMPLE, DRAWBAR (stand-alone, from `clipper-smk` host sim) and estimate cycles per
  block vs the ~2.9 ms deadline; (3) estimate arena sizes for 4 simultaneous heavy parts; (4) measure arena init time;
  (5) propose final budgets and the heavy-engine concurrency rule.
- Output: `DOCs/notes/T0.3-budget.md` + update 02 section 6 table and risks R-02/R-03/R-05.
- Accept: documented go/no-go per engine; numbers reproducible via a script in `/src/tools/`.

## T0.4 Input mapping spike (knobs, K8 master)

- Depends: T0.2
- Read: 03 section 1, `SMKSYNTH/firmware/src/platform.c`, `smk_control.h`, `obv_smk.h`.
- Do: locate encoder reading + virtual-pot code for K1 (MASTER) and K2-K8; also find how the physical sustain input is read (D-44) and expose it to the logical input layer; design `lgs_input.h` giving logical
  names to physical encoders (K1-K4 params, K5-K7 nav, K8 master); prove in the panel check screen and sim; keep the old
  mapping behind a compile flag for A/B comparison.
- Accept: sim shows K8 as MASTER with 24x1024 steps and 70% start; physical index table documented.

## T0.5 Safety audit (done)

- Depends: T0.2
- Done: `DOCs/safety-audit.md` (every write call site, the effective allow-list, 7 findings, the SDK checklist). Gates added to the build:
  `check_provenance.py` and `check_flash_writes.py` run at the start of `lgs_build.py`; `tests/flash_safety_test.sh` proves that a tampered protected
  file, a new `fl_write` call site and a flash-map window crossing BTIF/USRTRIM are all refused. Main findings: first boot over SMKSYNTH data
  re-initialises the data bank (A-1); flash erases mask interrupts, so saves must happen only while the engine is quiet (A-2).

## T0.6 Simulator bring-up (done)

- Depends: T0.2, T0.1
- Done: `src/web/sim/{sim.c,build.sh,index.html,sim.js,sim.css,worklet.js}` (our own, following the SMKSYNTH/Makkuro37 pattern: the firmware's
  `lgs_app.c` on the host harness `tests/host.c`, zig cc -> wasm32 + Binaryen Asyncify, AudioWorklet). SMK-37 panel: 8 knobs (K8 master), 4 faders,
  16 RGB pads, 18 buttons + a CHORD helper (KNOB BANK + FADER BANK), 37 keys, computer keyboard, Web MIDI, state box. Tools: `brew install zig binaryen`.
  Headless firmware test `tests/sim_wasm_test.mjs` (10 checks) and a browser run (firmware clock ticks, OCT+ changes the program on the firmware's screen).
  Gaps in `DOCs/sim-gaps.md`. Run: `sh web/sim/build.sh && python3 -m http.server -d build/sim-site 8737`, open `/index.html?autostart`.
- Still to come in M1 (T1.11): scenario recorder/player, framebuffer-hash goldens, fault-injection buttons, labels that follow the firmware's logical functions.

## T0.7 Remove the sequencer (D-50): study done, walking skeleton next

- Study (done): `DOCs/notes/T0.7-sequencer-removal.md`. Carving the sequencer out of `obervave.c` is not viable (tracks = parts, 324 `seq_`
  refs); the voice/DSP module only touches it through 5 hooks. Chosen direction (needs owner OK): **replace the application glue** with a
  new lean shell and drop `obervave.c`.
- Sub-cards: **T0.7a (done)** `lgs_seq_stub.h` (5 hooks) + `lgs_app.c` skeleton that compiles, runs on the host harness and builds a package
  (image 97 KB, RAM 42 KB, pool 278 KB); **T0.7b** audio ISR + USB MIDI/audio + panel check ported as small units with tests; **T0.7c** ARP/CHORD/
  GLOBAL/FX pages ported; **T0.7d** delete `obervave.c`, `obv_seq.h`, `obv_endless.h`, `obv_perf.h`, pattern storage and old tests; record new
  image/RAM/pool numbers.
- Original plan (superseded by the study)
- Read: 00 (D-50), `SMKSYNTH/firmware/src/obv_seq.h`, `obervave.c` (pattern, rec, step, undo code), `smk_store.h`, `obv_perf.h`,
  `smk_midi_mode*.h`, `obv_endless.h`, `docs/CONTROL-AUDIT.md`.
- Do: (1) dependency study: list everything that uses `obv_seq.h` and the pattern/REC/step code (ENDLESS arp recording, DRUM-X
  grid, MIDI MODE, scene recall, backup/restore, `SLOTS_AT` pattern storage in FL_SMP, pad LED code); (2) decide per feature:
  removed, kept without SEQ, or kept only if cheap; (3) remove in stages, building and running the maintained control tests
  after each stage; (4) free the pattern buffers (`pat_q`, `pat_old`, `undo_pat`, ... ~14 KB each in the pool) and the pattern
  flash region from the allow-list only if the new storage does not need it; (5) leave SEQ/SEQ PLAY/SEQ REC buttons unused;
  (6) record the new numbers (image size, RAM, pool).
- Output: `DOCs/notes/T0.7-sequencer-removal.md` with the dependency table and before/after budget; the code change is made in `/src`.
- Accept: build passes all budget checks in `tools/build.py`; control tests for ARP, CHORD, drums, MIDI, wheels and sustain still
  pass; tests for sequencer, patterns, save focus/display (pattern) are removed or adapted with the reason noted.
