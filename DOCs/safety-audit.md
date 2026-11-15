# Safety audit (T0.5, 2026-11-14)

Scope: the code in `src/` as of the application shell (`lgs_app.c`), checked against `06-storage-and-safety.md` and the
`smk37_open` best practices. Facts come from reading and from the automated checks listed at the end; nothing was run on hardware.

## 1. Where flash can be written

All writes funnel into two primitives in `hal/fm1_flash.h` (protected, hash-checked): `fl_erase4k()` and `fl_write()`. Both disable
interrupts around the RAM-resident routines (`fl_erase4k_ram`, `fl_prog_ram`, placed in `.ram_text`), and **both refuse any range
that is not allowed by `FL_GUARD(off, n)`** = `FL_RANGE_OK` (the app's allow-list) **and** `!FL_PROTECTED` (an independent second net:
head, BTIF/USRTRIM, stock USR, key_mac). Erase must be 4 KiB aligned; a program must stay inside one 256 B page.

Call sites of the primitives outside `hal/` and `loader/` (enforced by `tools/check_flash_writes.py`, baseline in `tools/flash-write-sites.txt`):

| File            | Count | What                                                                                                                                                                       |
| --------------- | ----- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `platform.c`    | 18    | `sector_save` / `slot_save` (header written last, so a cut save stays invalid), the first-boot owner check, OTA staging hooks (`ota_erase`, `ota_prog`), voice-store hooks |
| `smk_restore.h` | 2     | SMKSYNTH bank restore (`b2_write`, `b2_erase`): the session never starts in our build (`b1_can_begin()` returns 0); deleted in T0.7d                                       |
| `lgs_store.h`   | 0     | the only place new writers may be added (T1.9)                                                                                                                             |

## 2. Effective allow-list of the current build (profile `OBV_VOICE_HW=1`)

| Window            | Use                                                                                                                                              | Source              |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------- |
| `0x55000-0x56FFF` | restore journal (erased, unused by us)                                                                                                           | `smk_voice_store.h` |
| `0x57000-0x9CFFF` | user-data bank: owner tag `0x57000`, pattern slots `0x58000..0x97FFF`, settings `0x98000`, programs `0x99000` (8 KB), favorites `0x9B000` (8 KB) | `smk_voice_store.h` |
| `0xB3000-0xB4FFF` | MIDI maps                                                                                                                                        | `smk_store.h`       |
| `0xBB000-0xBFFFF` | OTA staging                                                                                                                                      | `fm1_flash_map.h`   |
| everything else   | **refused** (the old "stable" sectors `0x9D000..`, `0xC2000..` are read-only here, asserted at compile time)                                     |                     |

Never writable: `0x00000-0x03FFF` head, `0xC0000-0xC1FFF` BTIF + USRTRIM (calibration), `0xF4000-0xFDFFF` stock USR, `>= 0xFF000` key_mac.
Compile-time `_Static_assert`s (`FL_WIN_SAFE`) reject any window that is not sector aligned or that touches those ranges; the app slot
(`0x50000` = 320 KiB from `0x4000`) must end before the data bank.

## 3. Findings

| ID  | Finding                                                                                                                                                                                                                                                                                                                        | Severity                      | Action                                                                                                                                |
| --- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| A-1 | **First boot on a unit that ran SMKSYNTH** (owner tag `SMK37DATASMK16P1` at `0x57000`): our tag differs, so `voice_store_prepare()` re-initialises the user-data bank `0x57000-0x9CFFF`. SMKSYNTH data is lost. Calibration (`0xC1000`) is not in reach.                                                                       | Medium (data loss, not brick) | Install guide warns; back up the SMKSYNTH bank first (P-01). The data region will be redesigned in T0.3/T1.9                          |
| A-2 | **Flash erase/program masks interrupts**: an erase stalls the audio and USB ISRs (typically ~50 ms, spec up to 400 ms per 4 KiB sector).                                                                                                                                                                                       | Medium (glitch)               | Saves are explicit user actions and only while the engine is quiet (`engine_quiet()`); batch writes; never write on knob moves (T1.9) |
| A-3 | The old backup/restore protocol (B1/B2) can erase `0x57000..` from USB.                                                                                                                                                                                                                                                        | Low (never starts)            | Kept disabled by `b1_can_begin() == 0`; removed in T0.7d                                                                              |
| A-4 | `recovery.c` / boot guard: warm resets within 30 s of boot count as failures; two failures enter an app-level recovery mode (USB update). It cannot rescue an image that fails before the application starts. The loader's USB rescue (hold OCTAVE- at power-on) and the stock M-UPGRADE return path are the real safety nets. | Info                          | Verify both on the dev unit before any further install (G2)                                                                           |
| A-5 | `platform_poll()` (which feeds the watchdog, serves USB/OTA and clears the boot guard after 30 s) must be called every few ms from the main loop.                                                                                                                                                                              | Info                          | Our shell calls it every pass; the UI must never block (rule for all M1 code)                                                         |
| A-6 | The app slot is 320 KiB because the data bank starts at `0x57000`. A larger slot needs a new data layout.                                                                                                                                                                                                                      | Info                          | T0.3                                                                                                                                  |
| A-7 | Flash JEDEC id accepted: `0xEB6014` or `0x856014` only; other chips disable saving (`flash_ok = 0`).                                                                                                                                                                                                                           | Info                          | keep                                                                                                                                  |

## 4. SDK best-practice checklist (06 section 6a)

| Practice (`smk37_open`)                           | Status in `src/`                                                                                                              |
| ------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| No flash writes from the audio ISR                | OK: no caller of any writer in the ISR path (`app_audio` -> `engine_audio`); writers are called from the main loop only       |
| ISR code/data live in RAM while flash is busy     | OK: flash routines are `.ram_text`; the build verifies RAM text has no calls into XIP (1,708 instructions, 0 calls)           |
| Render within the block deadline, count overloads | Partly: the shell counts `ui_render_us/peak/overruns`; not yet shown on a panel check screen (T0.7b)                          |
| Edit with the engine paused                       | N/A today; saves follow A-2                                                                                                   |
| Staged writes with CRC                            | `sector_save`/`slot_save` write the header last with a CRC; our own A/B slots come in T1.9                                    |
| MIDI routing prefs are not project data           | Design rule for T1.9 (performance records contain no MIDI routing)                                                            |
| Build-time validation of symbols and sizes        | OK: `tools/build.py` checks entry address, RAM text, image <= slot, RAM/pool budgets, hardware register access only in `hal/` |
| Keep a way back to stock                          | Unchanged: loader rescue (OCTAVE- at power-on) and M-UPGRADE with the factory package; documented in the install guide (M4)   |
| Heap >= 49 KiB                                    | N/A: no heap, static memory; headroom: RAM 56 KB, pool 66 KB                                                                  |

## 5. Automated gates (all run by `tools/lgs_build.py` or `tests/flash_safety_test.sh`)

1. `check_provenance.py`: hashes of `fm1_flash.h`, `fm1_flash_map.h`, `fm1_guard.h`, `fm1_xip.h` and `loader/*` (build fails if changed; verified by tampering).
2. `check_flash_writes.py`: no new call site of `fl_write`/`fl_erase4k` (verified by injecting one).
3. Compile-time: `FL_WIN_SAFE` asserts (verified: a data window extended into BTIF/USRTRIM does not compile).
4. `smk_flash_test.c`: map windows never reach head, BTIF/USRTRIM, stock USR or key_mac.
5. `tools/build.py` checks (RAM text, sizes, register access).

## 6. Device storage of favorites and performances (T1.9b, 2026-11-14)

- Image: one `db` struct (about 7.8 KB: 64 favorites, 112 performances) with magic, version, generation and a whole-image CRC-32 (`lgs_ab.h`).
- Two copies, A at `0x99000` and B at `0x9B000`, two 4 KiB sectors each, all inside the allow-listed user-data window `0x57000-0x9CFFF`. Writes go through `sector_save` (header written last) and therefore through the guarded `fl_erase4k` / `fl_write`; `lgs_store.h` has no direct writer call, so `flash-write-sites.txt` is unchanged.
- A save always overwrites the copy that is not the newest valid one. A copy is accepted only if both sectors load and the whole-image CRC matches, so a power cut leaves the old image or the new one, never a mix. Boot picks the highest valid generation; a stale or damaged content is sanitized (unknown patch ids emptied, values clamped); if nothing is valid the factory defaults are used.
- Written from the main loop only, and only when `engine_quiet()` and no key is down (the erase masks interrupts, A-2). "Saved successfully" is shown after the commit; "Saving..." while it waits; "Save failed" if a primitive refuses.
- Evidence: `tests/store_ab_test.c` (fake flash, cut at each of the 6 erase / data / header steps, damaged newest copy, both copies damaged, empty flash) runs in `tests/flash_safety_test.sh`; `tests/store_test.mjs` covers the simulator image (bad CRC, wrong size, stale id).
- Not yet exercised on hardware. First boot of our firmware on a unit that ran SMKSYNTH still re-initialises the user-data bank (A-1); the flash backup (P-01) remains the gate before any install.
- The engine was changed in three places for per-line controllers (`ctl_bend_off`, `ctl_vib_off`, `ctl_mod_cut` in `obv_engine.h`); no flash or hardware access involved.
