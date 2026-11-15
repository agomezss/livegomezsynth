# LiveGomezSynth 0.1.3

Custom live-performance firmware for the M-VAVE SMK-37 Pro. Identity `SMK-37 Pro_20000`, family `LIVEGMZ`.

**Status: not tested on hardware.** The simulator build is the reference; gates G2 (first install) and G3 (soak) are pending.

## Contents

- 436 patches in 15 categories (Pianos 12, E-Pianos 12, Organs 20, Strings 9, Winds 9, Choirs 7, Brass 15, Basses 16, Pads 19, Leads 16, SFX 8, Complex 29, Drums 5, Guitars 7, ICONIC 252)
- engines: drm 5, fm2 90, fm6 17, org 40, sp 72, syn 212
- package: `livegomezsynth-0.1.3-smk37.fwsc`, 651819 bytes, SHA-256 `638e096f931b55f0ebee2a8f926a4695bc9cfcccea041f1e09a8312228cbc17e`
- budgets: image 291540 of 327680 bytes, RAM 72976 of 98304 bytes

## Before installing

Read INSTALL.md. Back up first: the installer page saves an identity record and your current firmware package; the first start re-initializes the user-data bank.

## Known limits

- Sounds are first drafts built from parameters; they were level-matched by measurement, not judged by ear.
- Update package only: stock M-VAVE V016 keyboards need SMKSYNTH or a first-install package first.
- The installer's restore path is untested on hardware; M-UPGRADE with the factory package is the official way back.
