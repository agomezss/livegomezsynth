# Provenance

`/src` is our own project. Parts of it started as imported copies of GPL-3.0 code from the reference repositories in this
workspace. They are **not forks and not synced**: after import, files are ours to change, except where a rule says
"verbatim".

| Imported from                                                         | What                                                                                 | License      | Status                                                                                                        |
| --------------------------------------------------------------------- | ------------------------------------------------------------------------------------ | ------------ | ------------------------------------------------------------------------------------------------------------- | --- | ------------------------------------------------------------------------ | ----------------- | ------------ | ------------------------------- |
| `SMKSYNTH/firmware/hal/*`                                             | hardware abstraction for the SMK-37 (inputs, LCD, flash driver and map, USB, timers) | GPL-3.0-only | **verbatim, protected** (safety rule 1 in `DOCs/06-storage-and-safety.md`); hashes in `provenance-hashes.txt` |
| `SMKSYNTH/firmware/loader/*`                                          | update loader (the code that writes the app during an update)                        | GPL-3.0-only | **verbatim, protected**                                                                                       |
| `SMKSYNTH/firmware/src/*`, `app.ld`, `crt0.S`, `gen/*`                | OberVave/SMKSYNTH application: voices, UI, ARP, FX, drums, MIDI, storage             | GPL-3.0-only | ours to change (planned: remove sequencer, new UI, new storage)                                               |
| `SMKSYNTH/tools/{build.py,fm1pkg_make.py,lz4blk.py,get_toolchain.sh}` | build steps, package maker                                                           | GPL-3.0-only | ours to change; `lgs_build.py` is new; `build.py` got the `LGS_APP_SRC` switch                                |
| `SMKSYNTH/tests/host.c`                                               | fake FM-1 used by the host harness                                                   | GPL-3.0-only | ours to change                                                                                                |
| (new) `firmware/src/lgs_app.c`, `lgs_seq_stub.h`, `lgs_input.h`       | the application shell (replaces `obervave.c`, D-50)                                  | GPL-3.0-only | ours; `obervave.c` is kept only until the shell reaches parity (T0.7d)                                        | \n  | (new, pattern of the SMKSYNTH/Makkuro37/winampFM simulators) `web/sim/*` | browser simulator | GPL-3.0-only | ours; written for this firmware |
| `SMKSYNTH/web/install/{protocol.js,package.js,JieLi-Apache-2.0.txt}` | web installer protocol and package checks (Web MIDI SysEx) | GPL-3.0-only (JieLi parts Apache-2.0) | **verbatim**; `web/install/{service,app}.js` are ours |

Lineage (from the upstream notices): Felucca by Leo Kuroshita (Hügelton Instruments), SLOOP by isod89, OberVave by zvenson,
the SMK-37 board layer by M. Geisler (genkigenki), SMKSYNTH by Marcus Padrini, DRUM-X by JIANT, SUPER by Clipper/isod89.
Keep the SPDX/copyright headers in every file that carries one.

Import fidelity check (2026-11-14): building `/src` with the reference parameters
(`lgs_build.py --version 1.2.0 --id 19649 --family SMK16P1`) produced a package byte-identical to
`SMKSYNTH/releases/1.2.0/SMKSYNTH-1.2.0-update-SMK37-Pro.fwsc`
(`ae19658ed7160a12046db99fc2d3570aa10b653f7d1d9f17a3fa6dbf7ff1295c`).

To be imported later (from the references, with this table updated each time): Clipper engines (T2.x, GPL-3.0 / msfa
Apache-2.0 / DaisySP MIT / CC0 samples), sample/FM data pipelines, the browser simulator (T0.6).
