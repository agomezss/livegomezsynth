# LiveGomezSynth 0.1.12

Custom live-performance firmware for the M-VAVE SMK-37 Pro. Identity `SMK-37 Pro_20000`, family `LIVEGMZ`.

**Status: not tested on hardware.** The simulator build is the reference; gates G2 (first install) and G3 (soak) are pending.

## Contents

- 215 patches in 14 categories (Pianos 29, E-Pianos 22, Organs 26, Strings 11, Winds 3, Choirs 15, Brass 18, Guitars 7, Basses 12, Pads 23, Leads 23, SFX 5, Complex 16, Drums 5)
- engines: drm 5, fm2 49, fm6 10, org 24, sp 33, syn 86, voc 8
- package: `livegomezsynth-0.1.12-smk37.fwsc`, 651847 bytes, SHA-256 `8ceeecba8d8a10346d15bc0103a17eb2bc87fff0749b7741f7b0fcdc4815c2f8`
- budgets: image 291860 of 327680 bytes, RAM 74240 of 98304 bytes

## Before installing

Read INSTALL.md. Back up first: the installer page saves an identity record and your current firmware package; the first start re-initializes the user-data bank.

## Known limits

- Sounds are first drafts built from parameters; they were level-matched by measurement, not judged by ear.
- Update package only: stock M-VAVE V016 keyboards need SMKSYNTH or a first-install package first.
- The installer's restore path is untested on hardware; M-UPGRADE with the factory package is the official way back.
