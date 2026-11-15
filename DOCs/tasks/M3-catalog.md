# M3 - Catalog

Common "Read first": `04-catalog-spec.md`, `catalog/names.md`, `05-engines-spec.md` (macro maps).
Work is parallel per category; each agent edits only its category block in `catalog/catalog.yaml` and its data files.

## T3.1 Categories 02-04 (Piano, EP, Organ)

- Do: author all names in `names.md` per engine assignment, golden render each, level-match, status `tuned`.

## T3.2 Categories 05-09 (Strings, Wind, Choir, Brass, Guitar)

- Same as T3.1.

## T3.3 Categories 10-14 (Bass, Pad, Lead, SFX, Complex)

- Same as T3.1; Complex patches include ARP params.

## T3.4 Category 15 (Drums)

- Drums: only the 5 SMKSYNTH kits (D-46). No GM category (D-51).

Acceptance (all): generator validation passes, golden hashes recorded, no patch exceeds -6 dBFS peak or is below -18 dBFS
RMS-ish target (health test), macro map exercised, license note for any external data.
