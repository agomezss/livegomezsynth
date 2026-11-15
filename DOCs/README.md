# LiveGomezSynth - Project Docs

Custom firmware for the M-Vave SMK-37 Pro, focused on **live performance** (fast patch recall, 2-layer split, 2 solo
patches) with a deliberately simple, hard-to-get-lost UX. Synth depth is secondary.

Status: **grooming + spec complete, no firmware code written yet.** Next step is M0 (see `09-roadmap.md`).

## Reading order

| #   | File                                                     | Read when                                                                    |
| --- | -------------------------------------------------------- | ---------------------------------------------------------------------------- |
| 00  | [00-decisions.md](00-decisions.md)                       | Always first. Locked decisions and the answers given by the owner.           |
| 01  | [01-source-analysis.md](01-source-analysis.md)           | You need to know what to reuse from `SMKSYNTH`, `smk37_open`, `clipper-smk`. |
| 02  | [02-architecture.md](02-architecture.md)                 | Touching code structure, parts/voices, realtime rules.                       |
| 03  | [03-ux-spec.md](03-ux-spec.md)                           | Anything about controls, screens, pads, LEDs, state machine.                 |
| 04  | [04-catalog-spec.md](04-catalog-spec.md)                 | Patch catalog, IDs, categories, data model.                                  |
| 05  | [05-engines-spec.md](05-engines-spec.md)                 | Porting or tuning a sound engine.                                            |
| 06  | [06-storage-and-safety.md](06-storage-and-safety.md)     | **Anything touching flash, packaging, install. Mandatory.**                  |
| 07  | [07-simulator-spec.md](07-simulator-spec.md)             | Browser simulator.                                                           |
| 08  | [08-test-plan.md](08-test-plan.md)                       | Writing or running tests, release gates.                                     |
| 09  | [09-roadmap.md](09-roadmap.md)                           | Picking the next task.                                                       |
| 10  | [10-open-questions-risks.md](10-open-questions-risks.md) | Before starting a card, check it is not blocked by an open question.         |
| -   | [catalog/names.md](catalog/names.md)                     | The owner's raw patch name lists and the M1 starter set.                     |
| -   | [tasks/](tasks/)                                         | Self-contained task cards per milestone (one card = one agent session).      |

## Rules for agents working from these docs

1. Pick **one** task card. Read only the card, `00-decisions.md`, and the docs the card lists under "Read first".
2. Do not widen scope. If a card needs a decision that is not in `00-decisions.md`, stop and add it to
   `10-open-questions-risks.md` instead of guessing.
3. Anything that writes flash, changes the loader/boot/OTA path, or changes the package layout must follow
   `06-storage-and-safety.md`. When in doubt: do not touch it.
4. All new firmware code goes under `/src`, our own project (not a fork, D-52). Never edit `SMKSYNTH/`, `smk37_open/`,
   `clipper-smk/`: they are references only. Record anything imported from them in `src/PROVENANCE.md`.
5. Every card ends with its acceptance checks passing and its status updated in `09-roadmap.md`.
6. Docs and code comments in English; device UI strings in English; license GPL-3.0-only.

## Glossary

| Term        | Meaning                                                                                          |
| ----------- | ------------------------------------------------------------------------------------------------ |
| Part        | One of 4 internal sound parts (NPART=4 in SMKSYNTH). Mapped to LAYER 1, LAYER 2, SOLO 1, SOLO 2. |
| Line        | A row of a performance: LAYER 1, LAYER 2, SOLO 1, SOLO 2. Line N = part N-1.                     |
| Patch       | One immutable factory timbre with a stable numeric ID (001..).                                   |
| Tweak       | The 4 knob adjustments (K1-K4) applied on top of a patch. Temporary unless saved in a favorite.  |
| Favorite    | Pad slot in PATCH mode holding patch ID + tweaks + FX sends. 4 banks (A-D) x 16 pads = 64.       |
| Performance | Combination of up to 4 lines (patch + range + volume + flags). 8 banks (A-H) x 14 slots = 112.   |
| Engine      | A DSP voice type, identified by a prefix: SYN, FM, FM2, SP, ORG, SVF, PLY, LAD, DRM.             |
| SMKSYNTH    | `/SMKSYNTH`, base firmware (OberVave port for SMK-37, GPL-3.0).                                  |
| SDK         | `/smk37_open`, official M-Vave SDK, reference for hardware facts and best practices.             |
| Clipper     | `/clipper-smk`, donor of extra engines (GPL-3.0).                                                |
