# Installing LiveGomezSynth

**Not tested on hardware yet.** Read all of this before you plug anything in. Everything here is reversible only as far as the way back (see "Going back") works on your keyboard.

## What you need

- An M-VAVE SMK-37 Pro, a USB cable, a computer with desktop Chrome or Edge.
- The installer page of this release (`site/install/`), served over HTTPS (GitHub Pages works) or from `localhost`. Web MIDI with SysEx needs one of the two.
- The firmware package of what the keyboard runs now (see step 1).

## 1. Find out what the keyboard runs

Open the installer page and press **Connect and identify**. It shows the identity the keyboard reports:

| Identity | Meaning | What to do |
| --- | --- | --- |
| `SMK-37 Pro_19615` ... `19649` | SMKSYNTH | go on |
| `SMK-37 Pro_2xxxx` | LiveGomezSynth | go on (an update) |
| `SMK-37 Pro_016` | the M-VAVE firmware | **stop**: this release is an update package. It needs SMKSYNTH (or a first-install package, not built yet) on the keyboard first |
| `ota-SMK-37 Pro_900` | an earlier transfer was cut off | the page resumes it |
| anything else | not supported | nothing is sent |

## 2. Back up

The update protocol only lets the keyboard pull a package: **this page cannot read the keyboard's flash**, so there is no flash dump. What it does instead:

1. **Identity record**: a small file with what the keyboard reported and when. Keep it.
2. **Restore kit**: you choose the package of the firmware you run now (the factory `.fwsc` from M-VAVE's M-UPGRADE for stock, or your SMKSYNTH `.fwsc`). The page checks its structure and identity, saves a copy in the browser and lets you download one. Keep the download.
3. **Your SMKSYNTH banks and favorites**: the first start of LiveGomezSynth re-initializes the user-data bank (`0x57000..0x9CFFF`). Export them with SMKSYNTH's own backup page first.

The calibration data at `0xC1000` is never written by this firmware or its loader (the build checks a write allow-list). The page cannot copy it either; that is why the official way back matters.

## 3. Install

1. Close other programs that use MIDI. Plug the keyboard in directly, not through a hub.
2. Tick the two confirmations, press **Install**, type `INSTALL`.
3. Do not unplug, sleep the computer or close the page. The keyboard restarts into its update loader, the package is written, the keyboard restarts again. The page ends with "Installed" and the new identity.
4. If the transfer is cut off, the keyboard stays in update mode (`ota-SMK-37 Pro_900`): reconnect and press **Install** again, it resumes.

## 4. First start

- The first start initializes the user-data bank (on a keyboard that ran SMKSYNTH: its programs and favourites are cleared, its pattern slots are left alone). The panel check's `Overruns` row ends in `flash OK  s1` after that start (`s0` on later starts); `flash --` means the store was refused and nothing can be saved.
- Panel check: hold **KNOB BANK** for 0.7 s. It tests faders, wheels, pedal, buttons and voices and shows render time. Calibration must be reported as found; if not, stop and go back.
- Play a few categories (15 in the list, Favorites first, Drums last).

## If an update says "the device stopped after 0 requests"

Versions 0.1.5 and 0.1.6 refuse SMKSYNTH's data bank (`flash --` in the panel check) and then ignore the update command. Switch the keyboard off, hold **OCTAVE-** while switching it on (the loader starts, no audio), open the installer page of 0.1.8 or later (earlier pages refuse a loader nobody started in that browser), press **Connect and identify** (the plan says "The keyboard is in its update loader (rescue mode)"), save the identity record and press **Install**. The first start of the new firmware takes the bank over and later updates work from the running firmware.

## Going back

- **Official way**: M-VAVE's M-UPGRADE with the factory package. This is the documented return to stock and needs nothing from this project.
- **USB rescue**: hold **OCTAVE-** while powering on: the keyboard starts only its loader, and any installer can write a package.
- **Restore kit (experimental)**: on the installer page, open "Restore the previous firmware", type `RESTORE`. Same mechanism as the install, never run on hardware.

## If something looks wrong

Keep the page's log (**Download the log**) and the identity record, and do not retry blindly. A keyboard that does not start can still be rescued through the loader as above.
