# 06 - Storage, Packaging and Brick-Avoidance (MANDATORY)

The SMK-37 holds **unique, non-recoverable data** in flash: factory pad/wheel/key calibration (`trimdat`, 0xC1000
region), Bluetooth pairing, key MAC/chip key data. Losing it degrades the device permanently. This document defines the
rules every change must satisfy. Values marked **VERIFY** are taken from SMKSYNTH docs and must be re-read from
`firmware/hal/fm1_flash_map.h` and `firmware/src/smk_store.h` in T0.1.

## 1. Flash map (inherited from SMKSYNTH)

| Region                                   | Use                                           | We may write?                          |
| ---------------------------------------- | --------------------------------------------- | -------------------------------------- |
| below 0x57000                            | boot, config, EQ                              | **Never**                              |
| app XIP slot (app.ld XIP LENGTH 0x8DFBC) | firmware app                                  | Only via the existing loader/installer |
| 0x9D000.. (10 sectors)                   | settings + user presets                       | Yes, allow-listed (family `LIVEGMZ`)   |
| 0x9E000-0xA5FFF                          | former SEQ patterns (sequencer removed, D-50) | Free for our records, allow-listed     |
| 0xAF000                                  | owner tag `SMK37DATA` + family                | Yes, first-boot only                   |
| 0xBB000-0xBFFFF                          | OTA staging                                   | Only by the loader                     |
| 0xC1000..                                | calibration, BT, USB key                      | **Never** (FL_GUARD)                   |
| USR trim/stock/key_mac                   |                                               | **Never**                              |

Rule: no new write target is added without (a) an entry in the allow-list, (b) proof it does not intersect any guarded
range (build-time check, see 6), (c) review against this table.

## 2. Non-negotiable rules

1. **Do not modify the loader, boot, OTA, flash driver, allow-list or guard** unless a card says so explicitly. Copy
   them verbatim from SMKSYNTH; `tools/check_provenance.py` compares the hashes of the flash-safety HAL files (`fm1_flash.h`,
   `fm1_flash_map.h`, `fm1_guard.h`, `fm1_xip.h`) and `loader/` with `src/provenance-hashes.txt`.
2. Calibration/BT/key regions are read-only. Keep both the allow-list and the independent `FL_GUARD` double check.
3. Package identity (`SMK-37 Pro_95XY` style string) must keep the real model name so the stock updater (M-UPGRADE)
   still accepts that model's factory firmware afterwards. The installer must refuse packages for other models.
4. Keep the **USB rescue**: holding OCTAVE- at power-on starts loader-only mode (no audio). Test this on every hardware
   release candidate.
5. The documented way back to stock (M-UPGRADE with the factory `.fwsc`) must remain valid and be documented in the
   install guide. Never ship a build whose first boot touches anything outside our sectors.
6. A power loss at any time must leave a bootable device and a readable (possibly older) user data set.
7. Never write flash from the audio ISR; mark the engine busy/muted around erases (`busy` hook); do not run code from XIP
   during flash writes (RAM text, as SMKSYNTH's build script enforces: "nothing may run from XIP").
8. `tools/build.py` checks (symbols in RAMTEXT do not call into XIP, `_start` at 0x02000120, XIP size <= slot) stay
   enabled. Add: budget checks and "no new flash write call sites outside `lgs_store.h`".
9. No automatic format-changing migration without a version field, CRC and a one-step fallback.
10. First boot with an unknown family clears **only** our own settings sectors (as SMKSYNTH). Installation docs warn:
    SMKSYNTH settings/patterns are erased when switching families (D-38); back up first with the SMKSYNTH backup tool or
    export manually.

## 3. User data format (new records, family `LIVEGMZ`)

- Region header: magic `LGSD`, format version `u16`, record count, CRC32 of header.
- Two **A/B slots** (ping-pong, each >= 1 sector set): the newest valid slot (highest sequence number + valid CRC32 of
  the whole slot) wins. Writes go to the _other_ slot, then the sequence number is committed last. Power loss mid-write
  leaves the previous slot intact.
- Records are fixed-size (see 04): favorites (64), performances (112), globals. Each record carries `crc16`; the slot
  carries `crc32`.
- Wear: favorites/performance saves are infrequent (user action), but cap saves to 1 per 2 s and never write on
  knob moves. Consider wear-level rotation across >= 4 sector groups if M0 measurement shows <= 10k erase endurance
  assumptions are at risk (VERIFY chip spec).
- Parser rules: bounds-check everything, reject unknown future versions (fall back to defaults + message, never crash),
  never trust `patch_id` (invalid ID -> empty).
- Fuzz and fault-inject (T1.9 acceptance): flip any byte, truncate at any offset, power-cut at each step.

## 4. Release gates

| Gate            | Criteria                                                                                                                                                         | Who      |
| --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------- |
| G0 host         | `tests/run_tests.sh` green, golden renders, budget script, storage fault-injection                                                                               | CI/agent |
| G1 simulator    | wasm sim scenario tests green; owner validates UX in the browser simulator (D-28)                                                                                | owner    |
| G2 hardware dev | install on the dev unit **after the flash backup is done and verified (P-01)**; panel check shows calibration found; USB rescue tested; stock return tested once | owner    |
| G3 soak         | 2 h playing with worst-case performance; no overloads, no stuck notes, no UI freeze; save/load cycle 200x                                                        | owner    |
| G4 release      | package SHA-256 manifest, THIRD_PARTY/licenses, install guide with warnings                                                                                      | agent    |

No hardware step before G1. No tagged release before G3.

## 5. Install guide must include (T4.x)

1. Full flash backup procedure (per `fm1-to-smk37` kit doc: dump must contain text `trimdat` at 0xC1224).
2. What gets erased on first boot (our settings area only) and what is never touched.
3. USB rescue and the way back to stock (M-UPGRADE).
4. Model identity (Pro/Elite/MKE) must match.

## 6. Build-time checks to add (T0.5)

- Link-map assertion: every flash write path is inside the allow-list; allow-list ranges do not intersect guarded
  ranges (compile-time `_Static_assert` + script).
- Hash check of the protected files (`tools/check_provenance.py`).
- Package manifest includes SHA-256 of app, sim wasm, test fixtures (as `DP2-MANIFEST.json`).
- `render_ticks/peak_ticks/overloads` counters exposed on the panel check screen.

## 6a. SDK best practices re-check (`smk37_open`)

Checklist to run once in T0.5 against our code base: ISR constraints; volatile-RAM placement of ISR-touched data; heap >= 49
KiB equivalent headroom (POOL/RAM); engine pause before bulk edits; MIDI routing preferences not stored in performance
data; staged writes with CRC; symbol/size validation; documented recovery. Record findings in `DOCs/safety-audit.md`.
