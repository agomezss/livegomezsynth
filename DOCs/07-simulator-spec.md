# 07 - Browser Simulator Specification

Goal: the owner validates UX, flows and sound in a browser before any device flash (D-28, gate G1).

## 1. Approach

Extend SMKSYNTH's `web/sim` (see 01): the **same firmware C code** compiled to WebAssembly with Zig
(`wasm32-freestanding`, bulk-memory), Binaryen Asyncify so the firmware main loop runs unmodified, an AudioWorklet that
calls the firmware audio render, a Canvas for the 240x240 RGB565 screen, localStorage-backed flash image.

Local setup: `zig` present; **install Binaryen** (`brew install binaryen`) for `wasm-opt`. Serve with
`python3 -m http.server -d <out> 8737`. Host-mode macro `OBV_HOST` provides fake HAL inputs (SMKSYNTH `tests/host.c` and
`web/sim/sim.c`).

## 2. Required UI (SMK-37 panel replica)

- 240x240 canvas (scaled 2x), pixel-exact from the framebuffer.
- 8 knobs K1-K8 as endless encoders (drag/wheel, +/- step buttons), labeled by _current logical function_ from a
  firmware-exported table so the replica follows D-03 automatically.
- 4 faders with soft-takeover indicator.
- 16 RGB pads with the real LED colors from the firmware LED state; click = tap, hold 700 ms = hold, shift-click = hold.
- Buttons: KNOB BANK, FADER BANK, PATCH, arrows, ARP, CHORD, GLOBE, FX, PARA, ENV, NOTE REPEAT, SCALE, BT,
  OCTAVE-/+, plus a **CHORD** toggle (press KNOB BANK+FADER BANK together) since a mouse is single-pointer; keyboard
  shortcuts for chords.
- 37-key on-screen keyboard + computer-keyboard mapping + Web MIDI input; velocity slider; pitch/mod wheel widgets;
  sustain toggle.
- Audio: AudioWorklet, level meter, CPU load estimate from firmware counters (informative only; real CPU is validated on
  hardware).

## 3. Developer/validation panel (collapsible)

- Live state: mode, bank, selected line, parts' patch IDs/engines, ranges, solo state, tweaks, sends.
- Event log of input events and mode transitions.
- Scenario recorder/player: JSON event scripts with timestamps (button/knob/pad/key) used by tests (08) and for
  reproducing bug reports.
- Screenshot capture (framebuffer PNG) and **framebuffer hash** display for UI golden tests.
- Storage view: A/B slot state, sequence numbers, CRC status; buttons "corrupt slot A", "cut power during next save",
  "factory reset" (fault injection for 06 section 3).
- Import/export of the simulated flash user data as a file (same format as hardware).

## 4. Parity rules

1. No simulator-only behavior in the firmware code path; differences live in HAL shims only.
2. Anything the sim cannot emulate must be listed in `DOCs/sim-gaps.md` (e.g. real flash timing, CPU deadline, LED
   brightness, pressure pads).
3. Sim build is produced from the same commit as the package; the build embeds the git hash and package SHA.
4. The sim must be able to load the engines added in M2 (size budget for WASM: informational, target < 2 MB gz).

## 5. Deliverables

- `src/web/sim/{build.sh,index.html,sim.js,sim.css,worklet.js,sim.c}` extended.
- `src/web/sim/scenarios/*.json` (core flows from 03: PATCH browse/save, PERFORM with solo, EDIT + popup, chord timing).
- Doc: `src/web/sim/README.md` with run instructions.
