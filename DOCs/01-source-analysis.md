# 01 - Source Analysis

Findings from reading the three reference projects. Items marked **VERIFY** must be re-checked in M0 before relying
on them.

## 1. SMKSYNTH (base) - `/SMKSYNTH`

- OberVave (zvenson) ported to SMK-37 on the **Felucca / SLOOP / FM-1 platform** (`firmware/hal`, `ota.c`, `lcd.c`,
  `loader/`). Toolchain: JieLi AC79 `pi32v2` clang (`tools/get_toolchain.sh`). Build: `BOARD=smk37 ./build.sh` ->
  `build/obervave-smk37.fwsc`. Config of record: `smk37-port.yaml`.
- Monolith C: `firmware/src/obervave.c` (3.3k lines) includes header-style modules: `obv_engine.h` (voices/DSP),
  `obv_ui.h`, `obv_smk.h` (SMK pads/faders/wheels), `obv_perf.h` (performance/favorites page), `obv_seq.h` (4-track,
  16-step sequencer; **to be removed, D-50**), `obv_fx.h` (chorus/delay/reverb), `obv_multiengine.h` (per-part engine table),
  `obv_patches.h` (64 factory programs as sparse `P_xxx,value` lists), `smk_store.h` (flash layout), `smk_*.h`.
- **Part model:** `NPART` parts, `NTRK=4`, ANALOG + DRUM-X. `obv_engine_ops_t {name,start,render,hit,track_render,
reset,busy}` with `obv_engines[]` table and an ISR-owned state machine `READY/OUT/IN` per part (gain ramps, no
  silent substitution). **This is the hook to add engines.**
- **Controls (SMKSYNTH 1.2.0; the old `SMK37.md` is stale, see notes/T0.4):** K1-K4 = four screen parameters, K5 tempo/contextual,
  K6 part, K7 preset, K8 MASTER (endless virtual pot). Faders: bank A = CUTOFF/RESO/ENV AMT/RELEASE, bank B = CHORUS/DELAY/REVERB/MASTER; soft
  takeover (acts after first move) with a bar overlay (`smk_overlay`, `smk_bar`, 1.5 s). Pads = 16 sequencer steps,
  Performance/Favorites bank of 8, Split/Layer bank, Patterns/Save bank, Drum pads. KNOB BANK short cycles pad
  banks; long (700 ms) = panel check.
- **Display:** 240x240 RGB565, two fonts (16 px and 32 px). Terminus 8x16 suggests 30 columns x 15 rows **VERIFY**.
- **Storage:** owner tag `SMK37DATA` + family `OBERVAV` at 0xAF000; settings + 32 presets at 0x9D000 region;
  patterns at 0x9E000..0xA5FFF; OTA staging 0xBB000..0xBFFFF; calibration/BT/key data at 0xC1000 never touched
  (allow-list + `FL_GUARD`). **VERIFY** exact map in `firmware/hal/fm1_flash_map.h` and `smk_store.h`.
- **Linker:** XIP 0x02000120 length 0x8DFBC (~560 KiB), RAMTEXT 24 KiB, RAM 96 KiB, POOL 0x54000 (336 KiB), NOINIT.
  Current app ~170 KB -> ~390 KB headroom for code + samples **VERIFY with a real link**.
- **Packaging/anti-brick:** `.fwsc` SysEx package with model identity, SHA-256 manifests, resumable install, USB
  rescue (hold OCTAVE- at power-on = loader only), return path via M-VAVE M-UPGRADE. See 06.
- **Web simulator:** `web/sim` = same firmware C compiled with Zig (`wasm32-freestanding`) + Binaryen `wasm-opt`
  Asyncify, AudioWorklet output, Canvas screen, localStorage flash, Web MIDI. Run:
  `sh web/sim/build.sh && python3 -m http.server -d docs/sim 8737`. Local machine has `zig`, `node`, `python3`;
  **`wasm-opt` missing** -> `brew install binaryen`.
- **Tests:** a large suite in `tests/` (C host tests, `*_wasm_test.mjs`, installer tests, golden hashes).
- **Known limitations worth inheriting as constraints:** shared voice pool (8 ANALOG voices, 5 with SUPER); no TRS MIDI
  IN; VOICE (formant) engine experimental; iPhone USB audio glitches.
- **Lessons:** keep DSP separate from UI; preserve flash formats/calibration; proportional validation; physical CPU
  deadline validation still pending upstream (we must do it).

## 2. smk37_open (official SDK) - `/smk37_open`

- Official M-Vave SDK for SMK-37 Pro/Elite. `app/` = GPL-3.0 reference instrument (8-track sequencer, DX7, 303, 808,
  PSG, Pluck, Sampler), `sdk/` = **proprietary** binary SDK (`libfm1_engine.a`, headers, linker scripts, manifest),
  `third_party/` = MSFA/Dexed (Apache-2.0), Open303, DaisySP, emu2149 (MIT), `web/` = browser workbench and simulator.
- **Architecture differs from SMKSYNTH:** audio is `fm1_engine_render(int32_t *out, unsigned frames)` in the DMA ISR
  (128-frame blocks, ~2.9 ms, Q23 stereo, 44.1 kHz). SMKSYNTH instead carries the older Felucca HAL. We do **not**
  migrate to the SDK build; we use it as a specification and checklist (D-02).
- **Hardware facts (use for HAL cross-checks):** AC791N (PI32v2), 240x240 SPI LCD, 37 keys, 16 pressure pads, 8
  encoders, 18 buttons, 4 faders, 41 RGB LEDs, USB MIDI, DIN MIDI out, BLE MIDI. Flash 475 KiB in the SDK
  instrument layout (different from the SMKSYNTH app layout).
- **Best practices to enforce (from SDK docs; see checklist in 06):** never write flash from the audio ISR or with
  audio running unprotected; ISR code and data used while flash is busy live in volatile RAM sections; render must
  meet the ~2.9 ms deadline (monitor overload counters); keep runtime heap >= 49 KiB; MIDI routing prefs are device
  preferences, not project data; edit with engine paused; staged writes with CRC so an interrupted write keeps the
  last committed bank; build-time validation of required symbols and sizes; keep factory-recovery path intact
  (V016 `.fwsc` via workbench).
- **FM implementation:** MSFA/Dexed 6-op, 32 algorithms, 8 voices/track, 112-byte packed DX7 voices in flash; 16
  factory voices in `app/factory_dx7.inc` (extracted from the FM-1 recovery resource - **licensing of those voice data
  is unclear, do not ship them**, see risk R-07; the owner overrode this for the 128 voices in D-69).
- **Licensing:** app GPL-3.0 with SDK-linking exception; SDK binaries/boot resources proprietary but redistributable for
  SMK-37 use with notices (`LICENSE.md`, `THIRD_PARTY.md` must be retained).

## 3. clipper-smk (engine donor) - `/clipper-smk`

- Clipper 1.1 on SMK-37: 23 engines, 407 presets, **same Felucca family as SMKSYNTH** (same HAL, same app linker
  layout, XIP 0x8DFBC) -> engines port with low HAL friction.
- Engine interface: `engine_t` in `firmware/src/core.h` (note_on, render per voice, optional block/amp/done), 8
  voices per part, per-part arena union for heavy engines (FM6, PHYS, GRAIN), engine switch clears arena and fades
  the old engine.
- **Dual core:** Clipper splits voices across a second CPU core (`cpu1.c`, `fm1_cpu1.S`, 17 of 23 engines). Whether
  SMKSYNTH uses it is **unknown - VERIFY (T0.3)**. This changes the CPU headroom estimate dramatically.
- Engines of interest:

| Engine  | File                      | Use here                   | Notes                                                                                                                 |
| ------- | ------------------------- | -------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| SAMPLE  | `eng_sample.c`            | SP                         | IMA ADPCM, zones + loop state, 8 voices; factory samples are CC0 in `assets/samples-cc0` (VSCO-2 CE, VCSL, Sonic Pi). |
| DRAWBAR | `eng_drawbar.c`           | ORG                        | 9 additive drawbars, 16 registrations, PERC/CLICK/DRIVE, rotary (SLOW/FAST). Low CPU.                                 |
| JUNO    | `eng_juno.c`              | (optional SYN-like flavor) | DCO + BBD chorus.                                                                                                     |
| ANALOG  | `eng_analog.c`            | SVF/PLY/LAD flavors        | 8 filter models incl. SEM (Oberheim), Moog ladder, MS-20, diode.                                                      |
| FM6     | `eng_fm6.c`, `fm6_core.c` | FM                         | msfa DX7, 6 voices (`FM6_POLY`), reads `.syx` banks, arena-based.                                                     |
| DIGITAL | `eng_digital.c`           | FM2                        | 4-op, 8 algorithms: electric pianos, bells, basses.                                                                   |
| FMX     | `eng_fmx.c`               | (deferred)                 | 6-op routable, High CPU.                                                                                              |
| STRINGS | `eng_strings.c`           | Strings category           | 1970s string machine, Low CPU.                                                                                        |
| VOICE   | `eng_formant.c`           | Choir category             | Formant vowels.                                                                                                       |
| PHYS    | `eng_phys.c`              | (optional)                 | Plucked/bar/bell models, High CPU, 3 voices.                                                                          |

- **Licensing:** firmware GPL-3.0-only; msfa Apache-2.0 (attribution file `LICENSES/Apache-2.0-msfa.txt`);
  DaisySP/Rings MIT; CrispyZebra PHASE GPL-3.0; `assets/samples-cc0` CC0 (keep `ATTRIBUTION.txt`); some assets (icons,
  panel photo, drum sounds from `gen_waves.py`) are **proprietary to Hügelton Instruments - do not copy**.
- **Tests:** host golden renders per engine x preset, CPU baselines, voice budget tests (`tests/run_tests.sh`,
  `tests/hostsim.c`): reuse as the engine-port acceptance harness.
- Web is an editor (SysEx), not a simulator: nothing to reuse for simulation.

## 4. Reuse map

| Need                                                 | Take from                    | Action                                                                            |
| ---------------------------------------------------- | ---------------------------- | --------------------------------------------------------------------------------- |
| HAL, loader, OTA, installer, package maker           | SMKSYNTH                     | Fork unchanged except identity strings. **Loader/boot byte-identical.**           |
| ARP / CHORD / GLOBAL                                 | SMKSYNTH                     | Keep. Rewire pad ownership (D-04) and knob indices (D-03). SEQ is removed (D-50). |
| SYN engine, DRUM-X, FX                               | SMKSYNTH                     | Keep; add DIST send bus.                                                          |
| Multi-engine routing                                 | SMKSYNTH `obv_multiengine.h` | Register new engines in `obv_engines[]`.                                          |
| FM, FM2, SP, ORG, SVF/PLY/LAD flavors, STRINGS/VOICE | Clipper                      | Port behind `obv_engine_ops_t` via an adapter shim (T2.0).                        |
| Simulator                                            | SMKSYNTH `web/sim`           | Extend with new UI.                                                               |
| Safety checklist                                     | SDK docs + SMKSYNTH          | Consolidated in 06.                                                               |
