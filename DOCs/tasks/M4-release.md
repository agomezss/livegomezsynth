# M4 - Release

Common "Read first": `06-storage-and-safety.md`.

## T4.1 Licensing and naming review

- Do: assemble `THIRD_PARTY.md` (msfa Apache-2.0, DaisySP MIT, SMKSYNTH/Clipper GPL, CC0 sample attributions, SDK notices);
  review the final catalog for brand-like names (D-42); decide public naming.

## T4.2 Install guide and package

- Do: install guide (backup, first-boot erase scope, USB rescue, stock return), package manifest with SHA-256, release
  notes, simulator bundle with matching hash.
- Accept: another person can follow the guide without reading source.

## T4.3 G2/G3 hardware gates

- Do: run 08 section 3 checklist on the owner's unit; record results; fix blockers; tag release candidate only after G3.
