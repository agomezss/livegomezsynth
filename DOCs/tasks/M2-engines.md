# M2 - Engines

Common "Read first": `05-engines-spec.md`, `02-architecture.md` sections 3-4, T0.3 budget note.
Each port follows 05 section 3. Do not start before T0.3 has a go/no-go for the engine.

## T2.0 Engine adapter shim

- Depends: M1 done (G1), T0.3
- Do: `lgs_engine_adapter.h`: expose a Clipper-style engine as `obv_engine_ops_t` per part; arena per part; macro-map
  interface; engine ids `LGS_ENG_*`; golden harness for engines running inside the SMKSYNTH core.
- Accept: a trivial dummy engine registered and switchable without glitches; ISR rules respected.

## T2.1 FM (FM6 / msfa)

- Depends: T2.0
- Do: port `fm6_core.c`, `eng_fm6.c`; authored DX7 voice bank pipeline (`catalog/fm/`); cross-check against
  `smk37_open/third_party` msfa; Apache attribution; macro map; 8 starter patches (EP, Piano, Bell, Bass, Brass, Guitar, Wind).
- Accept: golden + health + CPU tests; arena measured.

## T2.2 FM2 (DIGITAL 4-op)

- Depends: T2.0
- Do: port `eng_digital.c`; templates for EP/bell/bass; macro map; 6 starter patches.
- Accept: as T2.1.

## T2.3 SP (SAMPLE)

- Depends: T2.0
- Do: port `eng_sample.c`; sample pipeline `catalog/samples/convert.py` (CC0 only, attribution kept); zone sets for piano,
  strings, flute, horns, vibes/marimba, bass, kit within the budget; 8 starter patches.
- Accept: flash budget respected; loop correctness test; golden renders.

## T2.4 ORG (DRAWBAR)

- Depends: T2.0
- Do: port `eng_drawbar.c`; registrations for catalog 04 names; rotary control via mod wheel; macro map; 8 starter patches.
- Accept: as T2.1.

## T2.5 SVF / PLY / LAD flavors

- Depends: T2.0 (or SYN only for first pass)
- Do: parameter sets over SYN/ANALOG with SEM, Prophet-style, Moog ladder voicing; 4 starter patches each.
- Accept: distinct audible character vs SYN; golden renders.

## T2.6 Optional engines (STRINGS, VOICE formant, JUNO)

- Depends: T2.0, owner priority
- Do: port per budget only if headroom exists.

## T2.7 Heavy-engine concurrency guard

- Depends: T2.1-T2.4, T0.3
- Do: implement the concurrency rule from T0.3 (message in EDIT, never silent substitution); stress tests with worst-case combos.
- Accept: 60 s stress without overloads in host sim; CPU table updated.

## T2.8 Hardware smoke (G2 for M2)

- Depends: T2.1-T2.5, T0.5
- Do: first hardware install procedure (after backup), measure CPU counters, document results.
- Accept: checklist in 08 section 3 passes.
