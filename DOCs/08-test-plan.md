# 08 - Test Plan

Proportional validation: pure logic gets exhaustive host tests; UI gets scenario + framebuffer-hash tests; audio gets
golden/health/CPU tests; storage gets fault injection; hardware gets a checklist.

## 1. Test layers

| Layer                   | What                                                                                                                                                                           | Tooling                                     | Gate  |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------- | ----- |
| Unit (host C)           | catalog generator output, range/split resolution, solo state machine, chord detection, favorites/performance records (CRC, A/B), macro maps, master virtual pot, soft takeover | `src/tests/*.c`, `tests/run_tests.sh` style | G0    |
| Scenario (wasm + node)  | full flows from 03 via event scripts; assert state + framebuffer hash + pad LED colors                                                                                         | `*_wasm_test.mjs` style                     | G0/G1 |
| Audio golden            | every engine x preset render hash; health (peak, DC, clipping, silence-at-end, voice release, no hang)                                                                         | `hostsim`                                   | G0    |
| CPU/RAM                 | worst-case combos, budget script, regression vs baseline (+25% fails)                                                                                                          | `check_budget.py`                           | G0    |
| Storage fault injection | power cut at every write step, byte flips, truncation, unknown version, empty flash                                                                                            | host flash model                            | G0    |
| Installer/package       | identity, SHA manifest, allow-list/guard intersection, loader hash identical to SMKSYNTH                                                                                       | existing installer tests adapted            | G0    |
| Hardware checklist      | panel check, calibration, LEDs, pads/knobs mapping, CPU counters, USB rescue, stock return                                                                                     | manual                                      | G2/G3 |

## 2. Required scenarios (each = a JSON event script + expected state)

1. Boot -> PATCH screen text, K8 sets MASTER, level starts at ~70%.
2. K1-K4 show overlays with engine labels; tweaks reset on patch load.
3. PATCH: category via arrows/K6, patch via K7, favorites tap/hold/confirm/timeout/other-pad cancel, 5 s and 2 s timings.
4. KNOB BANK cycles A-D; FADER BANK -> PERFORM cycles A-H; chord enters EDIT (single-press suppressed).
5. PERFORM: combo load atomic; solo enter/leave; combo pad while solo only exits solo; held notes released cleanly.
6. Ranges: octave-granular `lo_oct..hi_oct` (split, overlap, disjoint), octave shift interaction, note-off routing after mode change.
7. EDIT: K1 line, K2 patch, K3 low octave, K4 high octave, K5 line volume, page 2 (PARA) K5 sustain / K6 bend / K7 mod, pad category preselect, popup K1/pad1/pad2/arrows,
   exit prompt save/discard/timeout.
8. Faders: soft takeover on mode/line change; sends per line stored in performance.
9. ARP/CHORD/GLOBAL regressions: SMKSYNTH's existing tests still pass with K-index remap (sequencer tests are dropped, D-50);
   Drums patch pad behavior (D-04 / D-12).
10. Storage: save -> reboot -> load identical; corrupt slot A -> falls back to B; both bad -> defaults, no crash.

## 3. Hardware checklist (G2/G3)

- Flash backup done and verified (P-01: `trimdat` at 0xC1224) **before** first install; photo of panel check after install.
- Every knob K1-K8, fader, pad (RGB), button, wheel responds per 03; K8 endless behavior and 70% start.
- Install, power-cycle x10, USB rescue (OCTAVE- at power-on), reinstall, stock-return test once.
- CPU counters during worst-case performance: no overload in 15 min; soak 2 h (G3).
- Save/load 200 cycles of favorites and performances; yank power during 20 saves (on a sacrificial run).
- MIDI in/out, USB audio smoke test, sustain/bend/mod per line.
