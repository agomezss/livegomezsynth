# 05 - Engines Specification

## 1. Engine matrix

| Prefix | Engine                                                        | Source                            | Milestone   | CPU      | RAM/arena        | Flash                  | Notes                                                            |
| ------ | ------------------------------------------------------------- | --------------------------------- | ----------- | -------- | ---------------- | ---------------------- | ---------------------------------------------------------------- |
| SYN    | ANALOG (OberVave)                                             | SMKSYNTH                          | M1 (exists) | Med      | voice pool       | code                   | 2 osc, SUPER, filter morph, arp, unison                          |
| DRM    | DRUM-X (JIANT)                                                | SMKSYNTH                          | M1 (exists) | Med      | 2.7 KB host pool | code                   | 5 kits, 8 instruments                                            |
| FM     | FM6 (6-op, msfa)                                              | Clipper `eng_fm6.c`, `fm6_core.c` | M2          | High     | FM6 arena        | + voice banks          | Apache-2.0 attribution; 6 voices (`FM6_POLY`)                    |
| FM2    | DIGITAL (4-op, 8 algos)                                       | Clipper `eng_digital.c`           | M2          | Med-High | small            | low                    | EP/bell/bass character not reachable on 6-op FM                  |
| SP     | SAMPLE (IMA ADPCM)                                            | Clipper `eng_sample.c`            | M2          | Low      | zone state       | <= 160 KiB ROM samples | CC0 sets only; loop state stored                                 |
| ORG    | DRAWBAR                                                       | Clipper `eng_drawbar.c`           | M2          | Low      | small            | low                    | 9 drawbars, 16 registrations, PERC/CLICK/DRIVE, rotary SLOW/FAST |
| SVF    | ANALOG filter model state-variable (SEM-like) + vintage drift | SYN + Clipper `eng_analog.c`      | M2          | Med      | -                | low                    | Flavor, not a new engine (D-35)                                  |
| PLY    | ANALOG/HYBRID flavor with poly-mod style params               | same                              | M2/M3       | Med      | -                | low                    | Stretch: dedicated engine                                        |
| LAD    | ANALOG ladder-filter model                                    | same                              | M2          | Med      | -                | low                    |                                                                  |
| (opt)  | STRINGS, VOICE (formant), JUNO                                | Clipper                           | M3          | Low      | -                | low                    | Serve Strings/Choir categories                                   |
| (def)  | FMX 6-op routable, PHYS                                       | Clipper                           | deferred    | High     | large            |                        | CPU/RAM risk                                                     |

An engine is **admitted** only after: golden render passes, CPU baseline recorded, worst-case polyphony measured on the
host benchmark, arena size documented, license note added to `THIRD_PARTY.md`.

## 2. Macro map (K1-K4)

Every patch points to a macro map: 4 entries `{label, target param(s), curve, min, max}`. K1-K4 move these values
(knob step 1/127 per detent with acceleration as in SMKSYNTH), with the overlay from 03 section 9. Defaults (SMKSYNTH's
PERFORM page parameters for subtractive engines):

| Engine                | K1                             | K2       | K3                              | K4      |
| --------------------- | ------------------------------ | -------- | ------------------------------- | ------- |
| SYN / SVF / PLY / LAD | CUTOFF                         | RESO     | ENV AMT                         | RELEASE |
| FM (6-op)             | BRIGHT (modulator level scale) | FEEDBACK | MOD ENV (modulator decay scale) | RELEASE |
| FM2                   | BRIGHT                         | FEEDBACK | MOD ENV                         | RELEASE |
| SP                    | CUTOFF                         | RESO     | ATTACK                          | RELEASE |
| ORG                   | DRAWBAR TILT (low<->high)      | PERC     | DRIVE / ROTARY speed            | RELEASE |
| DRM                   | TUNE                           | DECAY    | TONE                            | LEVEL   |

Rules: all four must be audible on every patch of the engine; where a parameter does not apply the macro map must
substitute a meaningful one, never a dead knob.

## 3. Port procedure (per engine) - T2.x

1. Copy only the engine files + dependencies into `src/firmware/src/engines/` keeping license headers.
2. Replace Clipper-global dependencies with the adapter (`lgs_engine_adapter.h`).
3. Register in `obv_engines[]`, add engine id to `LGS_ENG_*`.
4. Build host sim; run engine x preset golden renders; record CPU; check clipping/DC/release/hang tests (reuse
   `clipper-smk/tests/` approach).
5. Measure arena + XIP delta; update the budget table in 02 section 6.
6. Add at least 4 catalog patches and a macro map.

## 4. Engine-specific design notes

- **FM (6-op):** patches are DX-compatible 6-op voices. Do **not** ship voices extracted from the FM-1 recovery resource or commercial
  cartridges (R-07). Author voices (or use CC0 ones) and keep them in `catalog/fm/*.syx` -> packed bank.
- **FM2:** `DIGITAL` templates (EP, bell, bass) cover category 03 and parts of Guitar/Bass.
- **SP:** one-shot/looped zones from `clipper-smk/assets/samples-cc0` (VSCO-2 CE, VCSL, Sonic Pi are CC0). Pipeline
  `catalog/samples/convert.py`: WAV -> mono 22.05/32 kHz -> IMA ADPCM + loop ADPCM state -> blob + zone table. Keep
  `ATTRIBUTION.txt`. Budget first, then choose: piano (the largest), strings, flute, horns, vibes/marimba, bass, kit.
- **ORG:** registrations map to names in category 04 (e.g. `888000000`, `Full Drawbars`, `Jazz Organ`, `Gospel Organ`).
  Rotary slow/fast via mod wheel `DEFAULT` action on organ patches (speed toggle) - open design detail.
- **SVF/PLY/LAD:** begin as sparse param lists using a state-variable (SEM-like) filter (SVF), a ladder filter (LAD) and
  a poly-synth voice (stacked saws + poly-mod params, PLY). No brand names in UI or catalog (D-42).
- **DIST send:** new bus in `lgs_fx_dist.h` (parallel send, D-45; soft-clip/waveshaper with tone filter, oversampling-free, cheap). Parallel
  send like CHORUS/DELAY/REVERB, per part send level. Must not alias audibly or exceed deadline.
- **Arp in Complex:** SMKSYNTH ARP params are part of the program; no engine work needed.

## 5. CPU/RAM validation

`tools/check_budget.py` runs `tests/hostsim` benchmarks for: single engine, 2 layers (heavy+heavy), 4 parts (heavy x2
sounding + 2 muted solos), all FX on. Pass: <= 70% of deadline at the measured clock (VERIFY dual-core usage, T0.3), no
overloads in a 60 s stress run. Hardware validation on a device build is mandatory before M2 release (06).
