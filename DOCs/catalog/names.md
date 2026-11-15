# Catalog - live core set and backlog names

The owner's list from `IDEA.md` was a **suggestion** (D-43). This file has two parts: the **live core set**
(the commitment, ~105 patches, section A) and the **backlog** (the owner's original names, section B, brand-like names
already replaced per D-42). Names must fit 16 characters including prefix. Engines: `SYN`, `FM` (6-op), `FM2` (4-op),
`SP` (sampler), `ORG` (organ), `SVF`, `PLY`, `LAD` (analog flavors), `DRM`. IDs are assigned by `catalog.yaml`
(block per category, see 04).

## A. Live core set (proposed, adjust freely)

| Cat                   | Patches (prefix = engine)                                                                                                                              |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 02 Acoustic Piano (8) | SP Grand, SP Bright Grand, SP Upright, FM Honky Tonk, FM Pop Piano, FM Octave, FM Mellow Grand, SYN Chorus Grand                                       |
| 03 EP (10)            | FM2 Tine Soft, FM2 Tine Bright, FM2 Stage EP, FM2 Reed Soft, FM2 Reed Bite, FM2 Reed Trem, FM2 Elec Grand, FM EP 1, FM EP Bell, FM2 Dirty EP           |
| 04 Organ (10)         | ORG Full Drawbars, ORG 888000000, ORG Jazz, ORG Rock, ORG Gospel, ORG Church, ORG Pipe, ORG Perc, ORG Rotary Slow, ORG Rotary Fast                     |
| 05 Strings (6)        | SYN String Mach, SP Strings, SP Slow Strings, SYN Warm Strings, PLY Str Pad, SYN Pizzicato                                                             |
| 06 Wind (6)           | SP Flute, FM Alto Sax, FM Tenor Sax, FM Clarinet, FM Harmonica, SYN Accordion                                                                          |
| 07 Choir (4)          | SYN Choir Aah, SYN Choir Ooh, FM Vocal Aah, PLY Choir Pad                                                                                              |
| 08 Brass (8)          | SYN Brass Section, SVF Bright Brass, SVF Warm Brass, SYN Brass Stab, FM Trumpet, FM Trombone, SP French Horn, SVF Synth Brass                          |
| 09 Guitar (8)         | FM Steel Guitar, FM Nylon Guitar, SP Jazz Guitar, FM Clean Chorus, FM Muted Guitar, FM Funk Guitar, SYN Overdrive Gtr, SYN Power Chord                 |
| 10 Bass (10)          | FM Finger Bass, FM Pick Bass, FM Slap Bass, FM Fretless Bass, SYN Sub Bass, SYN Synth Bass 1, LAD Ladder Bass, LAD Acid Bass, FM Bass, SP Upright Bass |
| 11 Pad (10)           | SYN Warm Pad, SYN Soft Pad, SVF Sweep Pad, PLY Dream Pad, PLY Glass Pad, SVF Dark Pad, SYN Choir Pad, PLY Lush Pad, PLY Epic Pad, SYN Vintage Pad      |
| 12 Lead (10)          | SYN Saw Lead, SYN Square Lead, SYN Sync Lead, LAD Mono Lead, LAD Fat Lead, SYN PWM Lead, FM FM Lead, FM Pluck Lead, PLY Unison Lead, SYN Whistle Lead  |
| 13 SFX (6)            | SYN Rise FX, SYN Fall FX, SP Impact FX, SYN Laser FX, SYN Noise Burst, SYN Tape Stop                                                                   |
| 14 Complex (8)        | SYN ARP Warm, SVF ARP Bright, SYN ARP Pluck, PLY ARP Dream Pad, SVF ARP Seq Bass, SYN ARP Seq Lead, SYN ARP Chord, PLY ARP Trance                      |
| 15 Drums              | DRM kits 1-5 (SMKSYNTH JIANT kits, names from the kits)                                                                                                |
| 16 GM (removed)       | -                                                                                                                                                      |

In M1 (SYN and DRM only) the catalog is seeded from SMKSYNTH's 64 factory programs instead (04 section 7); the table
above becomes the M2/M3 target as engines land.

## B. Backlog (owner's original names)

Typos preserved except where marked. Brand-like names are replaced with generic ones.

## 02 - Acoustic Piano (15)

Chorus Grand (SYN), Grand Piano SAMP -> `SP Grand`, Bright Grand SAMP ->
`SP Bright Grand`, Mellow Grand, Ballad Grand, Pop Grand, Rock Grand, Upright Piano, FM Honky Tonk, SAMP Honky Tonk ->
`SP Honky Tonk`, FM Octave, FM Pop Piano, SAMP Pop Piano -> `SP Pop Piano`, FM Piano, FM Bright Piano

## 03 - EP (25)

Tine EP, Stage EP, Tine Soft, Tine Bright, FM EP, FM EP 1, FM EP 2, Elec Grand, EP Bell, Warm EP, Chorus EP,
Phaser EP, Tremolo EP, Crunch EP, Vintage EP, Sparkle EP, Detuned EP, LoFi EP, EP Pad, EP Strings, Reed Soft,
Reed Bite, Reed Trem, Dirty EP, Bright Reed

## 04 - Organ (27)

Prog Organ, Rock Organ 2, Jazz Organ, Rock Organ, Perc Organ, Full Drawbars, 888000000, Gospel Organ, Church Organ,
Pipe Organ, Cathedral Org, Baroque Organ, Theater Organ, Rotary Slow, Rotary Fast, Organ Perc, Dark Organ,
Bright Organ, Vintage Organ, Transistor Org, Combo Organ, Beat Organ, Pulse Organ, Soft Organ, House Organ,
Organ BassSplit, Organ Brass

## 05 - Strings (10)

Slow Strings, Warm Strings, Stereo Strings, Chamber Str, Orch Strings, Legato Strings, Staccato Str, Pizzicato Str,
Tremolo Str, Marcato Str

## 06 - Wind (26)

Flute, Ocarina, Pan Flute, Shakuhachi, Recorder, Piccolo, Alto Sax, Tenor Sax, Soprano Sax, Baritone Sax, Clarinet,
Bass Clarinet, Oboe, English Horn, Bassoon, Harmonica, Blues Harp, Accordion, Musette Acc, Bandoneon, Whistle,
Irish Flute, Ney Flute, Breathy Flute, Solo Sax, Wind Ensemble

## 07 - Choir (11)

Choir Aah, Choir Ooh, Mixed Choir, Boys Choir, Female Choir, Male Choir, Gospel Choir, Classical Choir, Vocal Aah,
Vocal Ooh, Voice Doo

## 08 - Brass (15)

Trumpet Solo, Muted Trumpet, Trombone Solo, Bass Trombone, French Horn, Horn Section, Brass Section, Bright Brass,
Warm Brass, Pop Brass, Staccato Brass, Marcato Brass, Swell Brass, Synth Brass 1, Synth Brass 2

## 09 - Guitar (25)

Steel Guitar, Nylon Guitar, Finger Guitar, Pick Guitar, 12str Guitar, Jazz Guitar, Clean Guitar, Chorus Guitar,
Muted Guitar, Funk Guitar, Overdrive Gtr, Distortion Gtr, Power Chord Gtr, Acoustic Strum, Ukulele, Mandolin, Banjo,
Reso Guitar, Slide Guitar, Harmonics Gtr, Guitar Pad, Clean Octave Gtr, Jazz Box Gtr, PalmMute Guitar, LoFi Guitar

## 10 - Bass (25)

Acoustic Bass, Finger Bass, Picked Bass, Fretless Bass, Slap Bass 1, Slap Bass 2, Pop Bass, Jazz Bass, Muted Bass,
Flatwound Bass, Sub Bass, Synth Bass 1, Synth Bass 2, Ladder Bass, Square Bass, Saw Bass, Acid Bass, Deep Bass,
Punch Bass, Bass Pick, Octave Bass, Rubber Bass, FM Bass, Growl Bass, Upright Bass

## 11 - Pad (25)

Warm Pad, Soft Pad, Analog Pad, Digital Pad, Air Pad, Choir Pad, String Pad, Sweep Pad, Slow Sweep, Dream Pad,
Glass Pad, Dark Pad, Bright Pad, Space Pad, Cosmic Pad, Pulse Pad, Motion Pad, Shimmer Pad, Velvet Pad, Noise Pad,
Detune Pad, Lush Pad, Epic Pad, Vintage Pad, LoFi Pad

## 12 - Lead (25)

Saw Lead, Square Lead, Sync Lead, Mono Lead, Porta Lead, Fat Lead, Mini Lead, Ladder Lead, Acid Lead, Soft Lead,
Hard Lead, Vox Lead, Flute Lead, Reso Lead, PWM Lead, FM Lead, Pluck Lead, Bright Lead, Dark Lead, Unison Lead,
Octave Lead, Whistle Lead, Solo Lead, Express Lead, Bend Lead

## 13 - SFX (25)

Rise FX, Fall FX, Impact FX, Hit FX, Boom FX, Sweep Up, Sweep Down, Noise Burst, Reverse Cym, Vinyl Stop, Tape Stop,
Laser FX, SciFi FX, Glitch FX, Spark FX, Wind FX, Thunder FX, Rain FX, Ocean FX, Helicopter, Explosion, Door Slam,
UI Click, Robot Vox, Cinematic Hit

## 14 - Complex (arp pad/lead) (25)

ARP Warm 1, ARP Warm 2, ARP Bright 1, ARP Bright 2, ARP Pluck 1, ARP Pluck 2, ARP Saw Pulse, ARP Sync Lead,
ARP Dream Pad, ARP Motion Pad, ARP Gate Pad, ARP Seq Bass, ARP Seq Lead, ARP Chord 1, ARP Chord 2, ARP Poly 1,
ARP Poly 2, ARP Trance 1, ARP Trance 2, ARP Berlin, ARP Analog Seq, ARP Digital Seq, ARP Echo Lead, ARP Hybrid Pad,
ARP Epic Seq

## 15 - Drums

Not listed by the owner. Decision D-46: only the 5 SMKSYNTH JIANT kits (DRM) for now; extra kits are future work.

## 16 - GM (removed)

Removed by D-51 (no General MIDI category).

## Engine-per-category cheat sheet (M2/M3)

| Category                    | Primary                             | Notes                                                                             |
| --------------------------- | ----------------------------------- | --------------------------------------------------------------------------------- |
| Acoustic Piano              | SP (Clipper SAMPLE, CC0 piano) + FM | SP patches share one or two piano zone sets with different filter/velocity layers |
| EP                          | FM / FM2                            | DX-style tines, bells; FM2 4-op for tine/reed/elec grand                          |
| Organ                       | ORG (Clipper DRAWBAR)               | registrations 888000000, Full Drawbars, Jazz, Gospel, rotary                      |
| Strings                     | SYN strings machine + SP            | Clipper STRINGS engine optional                                                   |
| Wind/Brass                  | FM + SYN + SP                       |                                                                                   |
| Choir                       | SYN / Clipper VOICE                 | formant optional                                                                  |
| Guitar                      | FM + SP                             |                                                                                   |
| Bass / Lead / Pad / Complex | SYN + SVF/PLY/LAD flavors           | ARP params baked into Complex patches (SMKSYNTH arp is a program parameter)       |
| SFX                         | SYN + SP                            |                                                                                   |
