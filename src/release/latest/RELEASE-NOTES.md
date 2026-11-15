# LiveGomezSynth 0.1.10

Custom live-performance firmware for the M-VAVE SMK-37 Pro. Identity `SMK-37 Pro_20000`, family `LIVEGMZ`.

**Status: not tested on hardware.** The simulator build is the reference; gates G2 (first install) and G3 (soak) are pending.

## Contents

- 444 patches in 15 categories (Pianos 12, E-Pianos 12, Organs 20, Strings 9, Winds 9, Choirs 15, Brass 15, Basses 16, Pads 19, Leads 16, SFX 8, Complex 29, Drums 5, Guitars 7, ICONIC 252)
- engines: drm 5, fm2 90, fm6 19, org 40, sp 70, syn 212, voc 8
- package: `livegomezsynth-0.1.10-smk37.fwsc`, 651819 bytes, SHA-256 `b64f1b1d96d79776f3f1d6f79fb3257ef8d27ef1d0b7f304f8e34b7f2b055581`
- budgets: image 296468 of 327680 bytes, RAM 74064 of 98304 bytes

## Before installing

Read INSTALL.md. Back up first: the installer page saves an identity record and your current firmware package; the first start re-initializes the user-data bank.

## Known limits

- Sounds are first drafts built from parameters; they were level-matched by measurement, not judged by ear.
- Update package only: stock M-VAVE V016 keyboards need SMKSYNTH or a first-install package first.
- The installer's restore path is untested on hardware; M-UPGRADE with the factory package is the official way back.
