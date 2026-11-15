# LiveGomezSynth simulator: owner review guide (G1)

The page runs the firmware's own C code (compiled to WebAssembly) on a simulated SMK-37 Pro. Nothing here touches hardware. The panel follows the photo of the real keyboard (wheels, screen, 8 knobs, 2 x 8 pads, 4 faders, button blocks, 37 keys); the screen is also shown enlarged below it.

Pad lights: **dark** = nothing assigned, **colour** = assigned, a **blinking** pad = selected (white in PATCH, green in PERFORM); the two solo pads are orange and the active solo blinks.

```sh
sh web/sim/build.sh
python3 -m http.server -d build/sim-site 8737      # then open http://localhost:8737/index.html?autostart
```

Computer keyboard: Z..M / Q..R rows play notes, `1` KNOB BANK, `5` FADER BANK, `7` PATCH, `-` PARA, `]` BT, arrows `<` `>`, Space = sustain pedal. Mouse: drag or scroll a knob; the CHORD button presses KNOB BANK + FADER BANK together. "Reset user data" clears the favorites and performances kept in this browser.

## Flows to try

1. **PATCH.** Boot lands in PATCH (favorites bank A). Tap a pad to load a favorite. Hold a pad 0.7 s, then press the same pad within 5 s to overwrite it. K6 changes category, K7 patch, K1-K4 tweak, K5 tempo. KNOB BANK again cycles banks A-D.
2. **Faders.** CHORUS, DELAY, DIST, REVERB are the sends of the current patch. After a patch change a fader does nothing until it crosses the stored value (soft takeover; the bar shows the target).
3. **PERFORM.** FADER BANK. Pads 1-7 and 9-15 load combos A-1..; pad 8 / pad 16 = SOLO 1 / SOLO 2 (solo mutes the layers; a combo pad while soloing only exits the solo). K6 bank, K7 slot. Play low and high notes: each layer sounds only inside its octave range; a solo plays on the whole keyboard.
4. **EDIT.** Press the CHORD button. Page 1: K1 line, K2 patch, K3/K4 low/high octave (layers only; solos have none), K5 volume. PARA toggles page 2: K5 sustain, K6 bend, K7 mod action (try the Pitch and Mod sliders and Space). Pads 1-15 jump to the first patch of a category; `<` `>` open the search popup. KNOB BANK asks Save (pad 1) or Discard (pad 2).
5. **BT** opens Save/Copy in any mode: K6 destination bank, K7 slot, pad 1 confirm, pad 2 cancel.
6. Reload the page: favorites and performances you saved are still there.
7. **Drum kits.** In PATCH, K6 two steps back from Favorites is the Drums category (one step back is ICONIC): the pads become drum hits (PATCH button toggles back to favorites).
8. **Pages.** ARP, NOTE REPEAT (LFO), SCALE (osc / filter), CHORD (envelopes), GLOBE (voice / output) and FX open parameter pages on K1-K4; press again to step through, the last press leaves. FX holds the global chorus, delay, reverb and the DIST send. Only the synth patches have the oscillator / filter pages.
9. **Panel check.** Hold KNOB BANK for 0.7 s. **MIDI**: Connect MIDI (notes, CC 1 / 7 / 64 / 123, bend, clock).

## Decisions for sign-off

- Is the PATCH / PERFORM / EDIT flow what you want to play live? What feels slow or risky on stage?
- Soft takeover on the faders: keep, or make the faders jump to their position?
- Mod actions FILTER / BRIGHT / SWELL / REVERB: names and ranges (FILTER opens the cutoff up to +64, BRIGHT up to +127, SWELL adds up to +27 level, REVERB raises the reverb send).
- Screens: wording, which rows to show, colours.
- Catalog: 436 patches (252 of them in ICONIC) (synth, FM, 6-op FM, organ, samples, five drum kits). Names, categories and sounds are first drafts; none has been tuned by ear yet.

Known gaps of the simulator: `DOCs/sim-gaps.md`. Automated checks: `sh tests/run_tests.sh`.
