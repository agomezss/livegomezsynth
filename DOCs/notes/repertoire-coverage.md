# Repertoire coverage (working list for timbre refinement)

Rule: a row says what the song needs, which catalog patches answer it today (names as on the panel; category in brackets), and what is
still weak or missing. "First draft" = made from parameters, not yet judged by ear. Update this after each listening session.

| Song / style | Needs | Patches today | Gap / to judge by ear |
| ------------ | ----- | ------------- | --------------------- |
| a-ha, Take on Me | bright plucky synth riff, synth bass, pad | LAD Pop Riff, SVF Pop Lead (Leads); LAD 80s Bass (Basses) | riff voicing (pulse + saw, delay) |
| Queen, I Want to Break Free | synth bass, string-synth, lead | LAD 80s Bass, SVF String Mach (Strings), SYN Square Lead | needs a quick check of the solo voice |
| Nightwish | piano, choir, harpsichord, leads, pads, bells, fantasy, strings | SP Grand Piano; SP Choir Swell / Choir Aahs / Glass Choir (Choirs); FM2 Harpsichord, FM2 Harpsi 2, SYN Harpsichord (Pianos); SVF Morph Lead, LAD Ladder Lead, PLY Lead; FM2 Bell Pad, FM2 Atmosphere, FM6 Soft Pad, SYN Neon Pad (Pads); FM2 Glocken, Celesta, Tube Bell, Music Box (Complex); PLY Orch Strings, PLY Strings, SP String Pad | orchestral hit, solo choir "ooh", real orchestral strings (samples are stabs only) |
| Deep Purple | overdriven tonewheel organ | ORG Purple Drive, ORG Tonewheel, ORG Rock Organ, ORG Dirty Organ | rotary speed switch is a patch choice (no live switch yet) |
| The Doors | combo organ (reedy, thin) | SYN Combo Organ, ORG Reed Organ | first draft |
| Classical piano | grand piano | SP Grand Piano, SP Soft Piano, FM2 FM Grand | one sample set: no velocity layers |
| M1-style piano | bright digital piano, house organ | FM2 House Piano, FM2 Digi Piano, ORG House Organ | judge against the real thing |
| Flute | concert flute | SP Soft Flute, SP Lofi Flute, ORG Soft Flute | |
| Pan flute | breathy flute | FM2 Pan Flute, SP Pan Flute | first draft (breath noise) |
| 80s (Eurythmics, Depeche Mode ...) | saw/pulse bass, pads, FM bells, stabs | LAD 80s Bass, SYN Pulse Arp, FM2 Trap Bell, FM2 Beat Stab, SVF Brass, PLY Brass | |
| RPM (Radio Pirata) | 80s pop leads, brass, organ | SVF Pop Lead, PLY Anthem Brass, SYN Combo Organ | |
| Van Halen, Jump | big detuned polysynth brass chord | PLY Anthem Brass, SYN Poly Brass | |
| Erasure, A Little Respect | pulsing bass, bright lead, bells | LAD 80s Bass, SVF Pop Lead, FM2 Bell Pad | |
| Michael Jackson, Beat It | metallic stab, synth brass | FM2 Beat Stab, SYN Brass Stab | |
| Europe, The Final Countdown | synth brass fanfare, string pad | PLY Fanfare, PLY Brass Swell, SVF String Mach | |
| Bon Jovi, 80s rock | organ, polysynth brass, pads, lead | ORG Rock Organ, PLY Anthem Brass, PLY Poly Pad, SYN Sync Lead | |
| Romantic ballads | EPs, fantasy pads, strings | FM2 Tine EP, FM2 Reed EP, FM6 Tine EP, FM2 Atmosphere, FM2 Bell Pad, PLY Orch Strings | |
| Latin rock (Mana, Enanitos Verdes) | piano, EP, organ, nylon guitar, accordion, strings | SP Grand Piano, FM2 Tine EP, ORG Slow Rotary, FM2 Nylon Guitar, ORG Accordion | acoustic steel guitar is an FM pluck only |

Known holes (not planned yet): orchestral hit, pipe organ, solo saxophone and trumpet, a real legato string section, electric guitar
voices. Say which of these matter and which patches above are not usable; the next round fixes those first.

## ICONIC (category 16, D-53)

125 patches, ids 449..: pianos and EPs, organs (Deep Purple / Doors / Toccata / cathedral), harpsichord, bells and fantasy sounds (Nightwish set), choirs, flutes and pan flutes, string and pad sets, brass (Jump, Final Countdown, arena, Beat It), 80s synth-pop leads, basses and pads (Take On Me, Eurythmics, Depeche Mode, A Little Respect, Radio Pirata), Latin rock keys and strings, and a few extras (Africa marimba, Axel F, Popcorn, chiptune, Oxygene-style lead, Kashmir-style orchestra). All defined in `tools/famous_defs.py`; the order in the instrument is `ORDER` there. Regenerate with `port_clipper_engines.py`, `gen_lgs_samples.py`, `gen_famous.py`, `gen_catalog.py`. Names hint at the song without using titles; the sounds are first drafts built from parameters and were never judged by ear.

Still missing: orchestra hit with a real sample, solo sax / trumpet, legato strings, steel guitar, electric guitar.

Second round (ICONIC, ids 574..): 86 more for rock, pop rock, rock nacional and Latin rock: rock/honky/stadium/power-ballad pianos, clav, Wurlitzer-style EP, clean/chorus/echo/12-string/steel/overdrive guitars, viola caipira, charango, Hammond-style organs, ska/reggae bubble organ, forro accordions, recorder/quena/shakuhachi/whistle, rock/ska/funk brass, strings and pads (Floyd, Cure, Phil Collins style), power-chord and solo leads, arps and basses. The category now holds 252 of the 255 entries it can hold.
