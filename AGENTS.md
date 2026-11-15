# AGENTS.md: handoff for AI agents and new contributors

Read this instead of the tree. It is the map, the rules and the traps. Specs live in `DOCs/` (numbered 00-10); decisions are `D-xx` in `DOCs/00-decisions.md` (the log wins over any older prose).

## What this is

Custom live-performance firmware for the **M-VAVE SMK-37 Pro** (JieLi AC79/pi32v2, 1 MiB flash, 320 KiB app slot, ~96 KiB RAM, 240x240 LCD). Our own project in `src/`, not a fork. **Never run on hardware**; the browser simulator (the same C compiled to WebAssembly) is the reference. Hardware gates are G2 (first install) and G3 (soak), see `DOCs/09-roadmap.md`.

## Language and style

- Code, comments, docs, UI strings: **English**. Chat with the owner: Portuguese, short.
- `GPL-3.0-only` SPDX header on every file; keep upstream copyright headers where they exist.
- Comments only when the code cannot say it. No emojis. Do not create new markdown files unless asked.
- macOS: BSD `sed` (no `\b`); use python for regex edits.

## Commands (from `src/`)

```sh
sh web/sim/build.sh                    # firmware -> build/sim-site/lgs.wasm (needs zig + binaryen)
sh tests/run_tests.sh [--quick]        # everything; --quick skips the device package build (needs the JieLi toolchain)
node tests/ui_scenarios.mjs            # UI behaviour (buttons, pads, LEDs, solo, edit...)
node tests/engines_test.mjs [--table]  # every patch sounds, no clipping, tails end; pitch checks
node tests/catalog_health.mjs [--trim|--update]   # levels, level match, golden renders
node tests/screens_dump.mjs [--update] # 10 screen goldens (tests/golden-screens.json)
node tests/fuzz.mjs 3 2000             # random panel and MIDI input
node tests/installer_test.mjs          # installer page logic against a fake keyboard
python3 tools/name_review.py           # D-42 name check
python3 tools/package_release.py --build   # checks, then release/<ver>/ and release/latest/ (version from src/VERSION)
```

Serve the simulator: `python3 -m http.server -d build/sim-site 8737`.

## Map of `src/`

| Path                                                                                   | What                                                                                                                        |
| -------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| `firmware/hal/`, `firmware/loader/`                                                    | **Protected, from SMKSYNTH.** Do not edit without the owner's OK (flash driver, update loader). One approved change: D-65. |
| `firmware/src/lgs_app.c`                                                               | The application unit: includes everything, defines the main loop.                                                           |
| `firmware/src/lgs_ui.h`                                                                | The whole UI: modes, buttons, knobs, pads, screens, LEDs (`leds_update`), popup, save dialog.                               |
| `firmware/src/lgs_model.h`                                                             | Data model: `sound_t`, `lgline_t`, `perf_t`, `db`; catalog index; `patch_build`; `part_apply`; `part_levels`; `route_mask`. |
| `firmware/src/lgs_store.h`, `lgs_ab.h`                                                 | Persistence: A/B image with CRC and generation; flash only through the guarded `sector_save`.                               |
| `firmware/src/obv_engine.h`                                                            | The voice core (OberVave): 8 voices, 4 parts, 44.1 kHz, 16-sample blocks, mixer with `ctl_mix_lvl` and `ctl_mix_trim`.      |
| `firmware/src/lgs_engine_adapter.h`                                                    | Clipper engines render 32-sample windows inside the 16-sample core (16-sample delay, `done` hook frees voices).             |
| `firmware/src/lgs_eng_{fm2,org,sp,fm6}.h`, `lgs_fm6_*.h`, `firmware/gen/lgs_samples.h` | Engines and preset tables. **Generated** (see tools).                                                                       |
| `firmware/src/lgs_flavors.h`                                                           | SYN parameter sets (`PF_*`, terminated by `X`) and `FACTORY_LGS[]`. Famous rows are generated between markers.              |
| `firmware/src/lgs_catalog_data.h`                                                      | **Generated** from `catalog/catalog.json`.                                                                                  |
| `firmware/src/lgs_input.h`, `lgs_midi.h`                                               | Logical control names (K1-K8, buttons); MIDI input.                                                                         |
| `catalog/`                                                                             | `catalog.json` (patches), `trims.json` (per-patch gain by name), `golden.json` (render hashes).                             |
| `tools/`                                                                               | Generators, build, release, review (below).                                                                                 |
| `tests/`                                                                               | See commands.                                                                                                               |
| `web/sim/`                                                                             | Simulator page (`index.html`, `sim.js`, `sim.css`, `worklet.js`) and `sim.c` (exports).                                     |
| `web/install/`                                                                         | Installer page; `protocol.js` and `package.js` are verbatim from SMKSYNTH.                                                  |

## Architecture in one paragraph

Three UI modes (PATCH, PERFORM, EDIT) over a user-data block `db` (64 favorites, 8x14 performances). A performance is 4 lines: layers 1-2 (octave range `lo..hi`) and solos 1-2 (**no range**, D-54). A line holds a catalog patch id and 4 tweak values; `part_apply` builds the engine patch (`patch_build`) into `live_p[part]`. Engines: SYN (OBV_ANALOG, id 0), DRUMS (2), FM2 (3, 4-op), ORG (4), SP (5, sampler), FM6 (6, 6-op msfa), VOC (7, vowel/formant voice for choirs); ids in `obv_engine_patch.h`. Each engine descriptor (`lgs_eng_t`) exposes 8 macro params; `knob_desc()` maps K1-K4. Engine params live in borrowed analog slots (`LGS_E_SLOT`), ADSR in `LGS_ENV_SLOT`.

## The catalog pipeline

`catalog/catalog.json` -> `tools/gen_catalog.py` -> `lgs_catalog_data.h`. Ids are immutable, category c owns `(c-2)*32+1 .. +32`, except category 16 (ICONIC) which owns 256 ids (449..704). Each entry: `{id, cat, engine, name, factory}`; `factory` = preset index in the engine (SYN: `< 64` is `FACTORY`, `>= 64` is `FACTORY_LGS`). `trims.json` (by **name**) becomes the `trim` field (64 = 0 dB).

**ICONIC** is generated from `tools/famous_defs.py` (SYN recipes, FM2/ORG/SP/FM6 rows, `ORDER`). To add or change timbres, edit the defs and run, in this order:

```sh
python3 tools/port_clipper_engines.py      # FM2 + ORG tables
python3 tools/gen_lgs_samples.py --venv ~/lgs-scratch/venv/bin/python   # SP table (needs numpy)
python3 tools/gen_famous.py                # SYN + FM6 rows, catalog entries of category 16
python3 tools/gen_catalog.py
sh web/sim/build.sh
node tests/catalog_health.mjs --trim ; python3 tools/gen_catalog.py ; sh web/sim/build.sh   # level match, then rebuild
node tests/catalog_health.mjs --update ; node tests/screens_dump.mjs --update
```

New ICONIC entries go **at the end of `ORDER`** so existing ids stay put. Then fix expectations that count things (`tests/engines_test.mjs` counts ORG patches).

## Limits that bite

- `cat_n` and `pat_idx` are `uint8`: a category holds at most **255** patches (ICONIC has 252).
- Patch `data` (preset index) is `uint8`: SYN indices are `64 + position` and must stay under 256.
- Names: 16 characters max, generic (**D-42**: no brands, models, artists; `tools/name_review.py` enforces; `ALLOW` needs a reason).
- Image budget 327,680 B (now ~290 KB); RAM 98,304 B (now ~73 KB). `lgs_build.py` prints both.
- `CTL=16` in the core vs 32 in Clipper engines: use `E32_CTL`.
- SYN time scale: `ms ~ 10000^(v/127)` (v=64 is ~100 ms, v=40 is ~18 ms); `P_FENV_AMT` and `P_ENV_O2` are centred at 64; `P_O2_SEMI` 24 = 0.

## Rules that protect the hardware (do not weaken)

1. Flash writes only through the guarded `sector_save` allow-list; `tools/check_flash_writes.py` and `flash-write-sites.txt` gate the build.
2. Never write the calibration/BT/key area (`0xC1000`). User data lives in `0x57000..0x9CFFF`, A/B windows `0x99000` / `0x9B000`.
3. `firmware/hal/` and `firmware/loader/` stay as imported (`tools/check_provenance.py` checks hashes); the only approved change is D-65 (the loader also keeps a bank tagged with our own family).
4. No flash access from the audio ISR; code used while flash is busy lives in RAM text.
5. Nothing is installed on a device before the restore kit is saved (`DOCs/install-guide.md`, `DOCs/06-storage-and-safety.md`).

## Installer and release

**Versioning rule:** `src/VERSION` is the single source (instrument APP_VER, package name, `version.json` shown on the simulator page). Every commit that changes firmware, simulator or installer bumps it once: `python3 tools/package_release.py --bump --build`, then commit `release/latest` with the sources. `.github/workflows/release.yml` publishes `release/latest` as the GitHub Release `v<VERSION>` on every push to main (a PR only checks it and uploads an artifact); `tools/check_release.py` refuses a `release/latest` that does not match `src/VERSION`.

`tools/lgs_build.py` builds app + loader + `.fwsc` (env: `JIELI_TOOLCHAIN`, `AC79_SDK`; Docker on macOS). Our package is an **update** package (like SMKSYNTH's): it installs on a keyboard that already runs SMKSYNTH (`SMK-37 Pro_19615..19649`) or us (`SMK-37 Pro_2xxxx`); stock V016 needs a first-install package that does not exist yet. `web/install/service.js` holds the source rules; `tests/installer_test.mjs` runs it against a fake keyboard. `tools/package_release.py` writes `release/latest/site` (simulator at `/`, installer at `/install/`), which `.github/workflows/pages.yml` publishes.

## Reference projects (not in this repository)

`SMKSYNTH/`, `smk37_open/` and `clipper-smk/` were kept beside `src/` while building and are gitignored. Everything needed to build and test is already in `src/`. You only need them to **regenerate engine data**: clone them next to `src/` (same parent folder).

| Folder         | Repository                                                                                          | Needed by                                                          |
| -------------- | --------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| `clipper-smk/` | https://github.com/davidozz0/clipper-smk (Clipper by isod89: https://github.com/isod89/clipper-fm1) | `port_clipper_engines.py`, `gen_lgs_samples.py`, `gen_fm6_data.py` |
| `SMKSYNTH/`    | https://github.com/4t59m4f784-creator/SMKSYNTH                                                      | reference only (provenance, installer protocol origin)             |
| `smk37_open/`  | https://github.com/m-vave/smk37_open                                                                | reference only (official SDK docs)                                 |

## Where decisions are

`DOCs/00-decisions.md` (D-01..D-54: names D-42, pad lights and solo range D-54, ICONIC D-53, no GM D-51, no sequencer D-50), `DOCs/03-ux-spec.md` (controls, modes, LEDs), `DOCs/04-catalog-spec.md`, `DOCs/05-engines-spec.md`, `DOCs/06-storage-and-safety.md`, `DOCs/safety-audit.md`, `DOCs/09-roadmap.md` (status board), `DOCs/10-open-questions-risks.md`, `DOCs/notes/repertoire-coverage.md` (what the owner plays).

## Working agreement with the owner

The owner works in large autonomous blocks and tests on hardware at the end. Keep the simulator and the tests green, say plainly what was not verified, and never claim hardware results. Sounds are first drafts: the owner judges them by ear, and fixes are parameter edits in `tools/famous_defs.py` (ICONIC) or the engine tables.
