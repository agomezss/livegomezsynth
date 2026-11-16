# LiveGomezSynth 0.1.14

Custom live-performance firmware for the M-VAVE SMK-37 Pro. Identity `SMK-37 Pro_20000`, family `LIVEGMZ`.

**Status: not tested on hardware.** The simulator build is the reference; gates G2 (first install) and G3 (soak) are pending.

## Contents

- 349 patches in 14 categories (Pianos 37, E-Pianos 22, Organs 34, Strings 19, Winds 14, Choirs 23, Brass 28, Guitars 23, Basses 28, Pads 31, Leads 32, SFX 5, Complex 48, Drums 5)
- engines: drm 5, fm2 49, fm6 138, org 24, sp 33, syn 92, voc 8
- package: `livegomezsynth-0.1.14-smk37.fwsc`, 651847 bytes, SHA-256 `ea0bfd65f98ebab852d1f317d2f3d34ab24aea3624732f68f64e7ae287106fca`
- budgets: image 319592 of 327680 bytes, RAM 74304 of 98304 bytes

## Before installing

Read INSTALL.md. Back up first: the installer page saves an identity record and your current firmware package; the first start re-initializes the user-data bank.

## Known limits

- Sounds are first drafts built from parameters; they were level-matched by measurement, not judged by ear.
- Update package only: stock M-VAVE V016 keyboards need SMKSYNTH or a first-install package first.
- The installer's restore path is untested on hardware; M-UPGRADE with the factory package is the official way back.
