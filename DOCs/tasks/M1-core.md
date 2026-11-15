# M1 - Core UX + simulator (SYN + DRM only)

Common "Read first": `00-decisions.md`, `03-ux-spec.md`. Open questions: none blocking (OQ-17 defaults apply; sequencer removed, D-50); resolved questions are in `10-open-questions-risks.md`.

## T1.1 Catalog pipeline and seed

- Depends: T0.2
- Read: 04.
- Do: `catalog/catalog.yaml` + `tools/gen_catalog.py` + generated `lgs_catalog_data.h`; seed with SMKSYNTH's 64 factory
  programs reclassified into categories (prefix `SYN`) and the 5 drum kits (`DRM`); accessors
  `lgs_cat_count`, `lgs_patch_by_id`, `lgs_patch_in_cat(cat, idx)`.
- Accept: generator validation tests (dupe IDs, block overflow, name > 16, unknown engine); host test loads every patch.

## T1.2 Logical input layer + mode state machine

- Depends: T0.4
- Read: 03 sections 1-2, D-11, D-13, D-50, T0.7 notes.
- Do: `lgs_input.h`, `lgs_modes.h`: modes PATCH/PERFORM/EDIT/POPUP/SAVE prompts, chord detection (120 ms, release-triggered),
  ARP/GLOBAL overlay return rule, KNOB BANK long = panel check. Remove Split/Layer and Mixer hooks per D-13; the sequencer is removed in T0.7 (D-50).
- Accept: unit tests for every transition in the diagram; scenario test 4 (08).

## T1.3 PATCH screen and favorites

- Depends: T1.1, T1.2
- Do: `lgs_patch_ui.h`, `lgs_fav.h`: screen per 03 section 3, category/patch navigation, load patch (resets tweaks),
  favorites tap/hold/5 s confirm/2 s message, empty slot flow, bank A-D.
- Accept: scenario test 3; timings verified with a fake clock.

## T1.4 PERFORM model, routing, ranges, solo

- Depends: T1.1, T1.2
- Read: 02 section 2, 03 sections 4, 6.
- Do: `lgs_perf.h`: performance model, atomic combo load, range resolution with explicit `lo_oct..hi_oct` (D-14, D-39), physical sustain input per line (D-44), part routing at note-on with stored
  destination for note-off, solo semantics D-05/D-23, sustain/bend/mod per line, active-line tracking.
- Accept: unit tests: split, overlap/pure layer, disjoint ranges, octave shift, solo enter/exit with held notes, non-solo combo pad while in solo only exits solo (D-16).

## T1.5 EDIT mode

- Depends: T1.4
- Do: `lgs_perf_ui.h` edit screen per 03 section 5: two pages toggled by PARA (D-49), K1 line, K2 patch, K3 low octave, K4 high octave (>= low), K5 per-line volume (not master), page 2 K5 sustain / K6 bend / K7 mod action, K8 global MASTER always; category preselect pads; save/discard prompt on exit.
- Accept: scenario test 7 (except popup).

## T1.5b Save/Copy dialog (BT)

- Depends: T1.3, T1.4, T1.9
- Read: 03 section 11, D-40, OQ-17.
- Do: BT dialog for PATCH (copy current patch+tweaks+sends to favorite bank A-D / slot 1-16), PERFORM (copy performance to bank A-H / slot 1-14) and EDIT (save working copy to a chosen destination, then exit EDIT). K6 bank, K7 slot, pad 1 blue confirm, pad 2 red cancel, overwrite summary, 10 s timeout, 2 s `Saved successfully`. BT has no other job.
- Accept: scenario tests for copy to other bank/slot, overwrite confirm, cancel, timeout; source unchanged after copy.

## T1.6 Search popup

- Depends: T1.5
- Do: popup per D-24/03: arrows change category, K1 navigates list, pad 1 blue confirm / pad 2 red cancel, FAVORITES first.
- Accept: scenario test 7 (popup part).

## T1.7 Pad LEDs and pad ownership

- Depends: T1.3, T1.4
- Read: 03 section 8, D-12, `SMKSYNTH` pad LED code in `obv_smk.h`.
- Do: LED colours per mode; Drums patch pad behavior with the PATCH toggle (D-12).
- Accept: LED color assertions in scenarios; drum pad tests of SMKSYNTH still pass.

## T1.8 FX sends and DIST bus

- Depends: T0.2
- Read: 05 section 4 (DIST), `obv_fx.h`.
- Do: `lgs_fx_dist.h` parallel bus; per-part sends for 4 buses; faders map to the active line sends with soft takeover and overlay.
- Accept: audio golden for DIST; CPU delta recorded; scenario test 8.

## T1.9 Storage (favorites, performances, globals)

- Depends: T0.5
- Read: 06 sections 1-3.
- Do: `lgs_store.h`: A/B slots, CRC16/CRC32, versioned header, fixed records (04), save throttle, audio busy handling.
- Accept: fault-injection suite (06): byte flips, truncation, power cut at every step, unknown version, empty flash.

## T1.10 Knob macros and overlays

- Depends: T1.1, T0.4
- Do: K1-K4 macro maps for SYN and DRM (05 section 2), overlay bar/labels, tweak reset on patch load, snapshot into favorite/performance.
- Accept: scenario test 2.

## T1.11 Simulator UI

- Depends: T0.6, T1.2
- Read: 07.
- Do: panel replica with logical labels, chord helper, developer panel, scenario recorder/player, fault injection buttons.
- Accept: owner can run the full 03 flow in the browser.

## T1.12 Test automation

- Depends: T1.3-T1.10
- Do: wire scenario scripts (08 section 2) + unit tests into `tests/run_tests.sh`; framebuffer hash goldens.
- Accept: G0 green.

## T1.13 G1 review package

- Depends: T1.11, T1.12
- Do: build the simulator bundle with a short owner-facing guide (`src/web/sim/README.md`) listing flows to try and the
  decision list for sign-off.
- Accept: owner sign-off on UX recorded in `09-roadmap.md`.
