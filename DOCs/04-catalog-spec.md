# 04 - Patch Catalog Specification

## 1. Categories and ID blocks (D-25, D-32)

IDs are `u16`, assigned from fixed per-category blocks so numbers never shift. IDs are never reused or renumbered.
Favorites (`F001..F064`) are not patches: they are slots that reference a patch.

| Cat | Name                   | ID block    | Capacity | IDEA.md target | Primary engines                 | Secondary              |
| --- | ---------------------- | ----------- | -------- | -------------- | ------------------------------- | ---------------------- |
| 01  | Favorites              | (F001-F064) | 64       | -              | -                               | -                      |
| 02  | Acoustic Piano         | 001-032     | 32       | 15             | SP, FM                          | SYN (chorus poly), FM2 |
| 03  | EP                     | 033-064     | 32       | 25             | FM, FM2                         | SYN, ORG               |
| 04  | Organ                  | 065-096     | 32       | 27             | ORG                             | SYN                    |
| 05  | Strings                | 097-128     | 32       | 10             | SYN (strings machine), SP       | PLY                    |
| 06  | Wind                   | 129-160     | 32       | 26             | FM, SP                          | SYN                    |
| 07  | Choir                  | 161-192     | 32       | 11             | SYN, FM                         | SP                     |
| 08  | Brass                  | 193-224     | 32       | 15             | SYN, SVF, FM                    | SP                     |
| 09  | Guitar                 | 225-256     | 32       | 25             | FM, SP                          | SYN                    |
| 10  | Bass                   | 257-288     | 32       | 25             | SYN, LAD, FM                    |                        |
| 11  | Pad                    | 289-320     | 32       | 25             | SYN, SVF, PLY                   | FM                     |
| 12  | Lead                   | 321-352     | 32       | 25             | SYN, LAD, PLY                   | FM                     |
| 13  | SFX                    | 353-384     | 32       | 25             | SYN, SP                         | FM                     |
| 14  | Complex (arp pad/lead) | 385-416     | 32       | 25             | SYN, SVF, PLY (with ARP params) |                        |
| 15  | Drums                  | 417-448     | 32       | 5 kits +       | DRM                             |                        |

The `IDEA.md target` column is the owner's original suggestion (279 names + Drums; the GM category was dropped, D-51). Per D-43 it is a
**backlog**, not the commitment. The commitment is the **live core set**:

| Cat               | Live core target | Engine variety to include                                                               |
| ----------------- | ---------------- | --------------------------------------------------------------------------------------- |
| 02 Acoustic Piano | 8                | SP grand/bright/upright, FM honky/pop/octave, SYN chorus grand                          |
| 03 EP             | 10               | FM2 tine/stage/reed (soft, bright, trem), FM bell, phaser, dirty (DIST), FM EP 1/2      |
| 04 Organ          | 10               | ORG: full drawbars, jazz, rock, gospel, church, pipe, perc, rotary slow/fast; SYN combo |
| 05 Strings        | 6                | SYN string machine, SP strings, PLY pad-strings                                         |
| 06 Wind           | 6                | FM/SP flute, alto/tenor sax, clarinet, harmonica, accordion                             |
| 07 Choir          | 4                | SYN/FM aah, ooh, mixed, vocal pad                                                       |
| 08 Brass          | 8                | SYN/SVF/FM section, bright, warm, stab, trumpet, trombone, horn                         |
| 09 Guitar         | 8                | FM/SP steel, nylon, jazz, clean chorus, muted, funk, overdrive, power chord             |
| 10 Bass           | 10               | SYN/LAD/FM: finger, pick, slap, fretless, sub, synth 1/2, acid, FM bass                 |
| 11 Pad            | 10               | SYN/SVF/PLY: warm, soft, sweep, dream, glass, dark, choir pad, lush, epic               |
| 12 Lead           | 10               | SYN/LAD/PLY: saw, square, sync, mono, fat, PWM, FM lead, pluck, whistle                 |
| 13 SFX            | 6                | rise, fall, impact, laser, noise burst, tape stop                                       |
| 14 Complex        | 8                | arp pad/lead/pluck/seq/chord presets (SYN/SVF/PLY with ARP params)                      |
| 15 Drums          | 5 kits           | the 5 SMKSYNTH JIANT kits (D-46)                                                        |
| (16 ICONIC) | removed (D-66) | the category of iconic-song timbres is gone; after a review of the whole list the ones kept live in the categories above; the ids are sequential in category order (D-68, D-69); the SDK's 128 FM voices and six sax/trumpet patches joined in 0.1.14 (349 patches) |

Total live core: ~105 patches + 5 kits. Every engine (SYN, FM, FM2, SP, ORG, SVF, PLY, LAD, DRM) must appear in at
least 4 patches, and each category with more than one engine shows variations distinguished by prefix. Header counts
in `IDEA.md` (e.g. "Acoustic Piano (10)" with 15 names listed) are inconsistent anyway; capacity 32 per block absorbs
growth. Display numbers in `IDEA.md` examples ("157 BRASS", "300 LEAD") are illustrative (D-32).

Display: patch number is 3 digits `001..448`; favorites `F001..F064`.

## 2. Prefixes

Patch name = optional prefix + name, <= 16 characters total. Prefix identifies the engine when the same timbre exists in
several engines (as in `IDEA.md`): `FM` (6-op), `FM2` (4-op), `SYN`, `SP`, `ORG`, `DRM`, plus analog flavors `SVF`
(twin oscillators + state-variable filter), `PLY` (poly synth with poly-mod style params), `LAD` (ladder filter).
These three replace `OBE`, `PRO`, `MOG` to avoid trademark-like prefixes (D-42); the mapping is a string table and
can be changed later. Unambiguous
timbres (e.g. `Rotary Slow` on ORG) may omit the prefix. The engine is always shown on the last row of the PATCH screen.
`IDEA.md` uses `SAMP`; this spec normalizes it to `SP`.

## 3. Source of truth: `catalog/catalog.yaml`

```yaml
version: 1
categories:
  [
    { id: 2, name: "Acoustic Piano", short: "Pianos", base: 1, capacity: 32 },
    ...,
  ]
patches:
  - id: 1
    cat: 2
    name: "SP Grand" # <= 16 chars, ASCII
    engine: SP
    data: { zoneset: piano_grand, macro: sp_default } # engine-specific payload ref
    tags: [acoustic]
    status: planned|draft|tuned|released
```

`tools/gen_catalog.py` validates (unique IDs, inside block, name length, engine exists, no duplicates) and emits C
tables.

## 4. Patch ROM record (generated, const in XIP)

```c
typedef struct {
    uint16_t id;        // stable ID
    uint8_t  cat;       // 2..15
    uint8_t  engine;    // LGS_ENG_*
    char     name[16];  // not NUL-terminated if 16 chars
    uint8_t  macro;     // macro-map index for K1-K4 (05)
    uint8_t  flags;     // bit0: has ARP params, bit1: mono, ...
    uint16_t data;      // index/offset into the engine's table (sparse param list, FM bank slot, zoneset, registration)
} lgs_patch_t;          // 24 bytes
```

Patch audio data: SYN = sparse `{P_xxx, value}` lists like `obv_patches.h`; FM = 6-op FM voice (156 B expanded / 128 B
packed; stored 112 B packed) in a bank blob; SP = zone set descriptors + ADPCM blobs; ORG = registration (9 drawbar
levels + perc/click/drive/rotary); SVF/PLY/LAD = SYN-style lists plus filter model id.

## 5. Favorite record (flash, 64 slots)

```c
typedef struct {
    uint16_t patch_id;   // 0 = empty
    uint8_t  tweak[4];   // K1-K4 values at save time (0-127)
    uint8_t  send[4];    // CHORUS, DELAY, DIST, REVERB (0-127)
    uint8_t  flags;      // reserved
    uint8_t  rsv;
    uint16_t crc16;
} lgs_fav_t;             // 16 bytes
```

## 6. Performance slot record (flash, 8 banks x 14 = 112 slots)

```c
typedef struct {
    uint16_t patch_id;   // 0 = line empty
    uint8_t  origin_fav; // 0 none, 1..64 = chosen from FAVORITES (display prefix F); informational only
    uint8_t  lo_oct;     // low octave 1..7: line starts at C of this octave
    uint8_t  hi_oct;     // high octave 1..7 (>= lo_oct): line ends at B of this octave (D-14)
    uint8_t  volume;     // 0-99
    uint8_t  flags;      // bit0 sustain, bit1 bend, bits2-4 mod action (0 DEFAULT,1 OFF,2 FILTER,3 BRIGHT,4 SWELL,5 REVERB)
    uint8_t  tweak[4];
    uint8_t  send[4];
    uint8_t  rsv;
} lgs_line_t;            // 16 bytes
typedef struct { lgs_line_t line[4]; uint16_t flags; uint16_t crc16; } lgs_perf_t;   // 68 bytes
```

Total records: 64 x 16 + 112 x 68 = ~8.6 KB plus globals; fits one record region with A/B ping-pong (see 06).

## 7. M1 starter set (~64 patches)

M1 has only SYN and DRM engines. Seed the catalog by **re-classifying SMKSYNTH's 64 factory programs** (`NFACT=64` in
`obv_patches.h`, e.g. FAT BASS, SWEEP PAD, SOFT KEYS, PULSE ORGN, HORN SECT, ARP CHORD, DARK ARP, SUB BASS, SYNC LEAD...)
into the categories above (prefix `SYN`) plus the 5 JIANT drum kits as `DRM` patches. Unclassifiable programs go to the
nearest category; none are renamed beyond the 16-char limit. Result: a realistic catalog that exercises every UI path
with zero sound-design risk. Task T1.1.

## 8. Naming/quality rules for new patches (M2/M3)

- Each patch needs: name (<= 16), engine, data, macro map, and a **golden render** (host sim) for regression.
- Provide at least one patch per category per engine used (so every K1-K4 macro map gets exercised).
- Names must not contain trademarks or brand-derived words (D-42): e.g. use `Tine EP`, `Reed EP`, `Elec Grand`, `Combo
Organ`, `Beat Organ`, `Ladder Bass`, `Pop Piano`, `Chorus Grand`, `FM EP 1`.
- Level-match patches (peak between -12 and -6 dBFS at velocity 100 for a single note) via the golden health test.
- Only use data we own, generated, or that is CC0/permissively licensed (R-07). No ripped ROM/dump patches; the one exception is the SDK's 128 FM voices (D-69, credited in THIRD_PARTY.md).

## 9. Raw name lists

The owner's name lists (by category) are preserved in [catalog/names.md](catalog/names.md) with proposed engine for
each category.
