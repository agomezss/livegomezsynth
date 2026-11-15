// SPDX-License-Identifier: GPL-3.0-only
// UI scenarios on the firmware compiled to WebAssembly (M1): modes, chord, favorites, performances, solo,
// routing by octave range, EDIT, save/copy dialog, soft-takeover faders.
//   sh web/sim/build.sh && node tests/ui_scenarios.mjs
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
let fail = 0;
const check = (name, ok, info = "") => {
  console.log((ok ? "  ok    " : "  FAIL  ") + name + (info !== "" ? "  (" + info + ")" : ""));
  if (!ok) fail++;
};
const run = (ms) => { for (let i = 0; i < ms; i += 2) ex.sim_render(88); };
const str = (p) => { const m = new Uint8Array(ex.memory.buffer); let s = ""; while (m[p]) s += String.fromCharCode(m[p++]); return s; };
const row = (r) => str(ex.sim_row(r));
const msg = () => str(ex.sim_msg());
const BTN = { KB: 0, FB: 1, LEFT: 2, RIGHT: 3, OCTD: 4, OCTU: 5, ARP: 6, LFO: 7, SEL: 8, ENV: 9, GLO: 10, SAVE: 11, HOME: 12, EDIT: 13, FX: 14 };
const press = (b, ms = 20) => { ex.sim_button(b, 1); run(ms); ex.sim_button(b, 0); run(30); };
const tap = (p) => { ex.sim_pad(p, 100); run(30); ex.sim_pad(p, 0); run(30); };
const pad = (p) => { ex.sim_pad(p, 100); run(30); };          // press only
const enc = (e, n) => { ex.sim_enc(e, n); run(20); };
const chord = () => { ex.sim_button(BTN.KB, 1); run(40); ex.sim_button(BTN.FB, 1); run(40); ex.sim_button(BTN.KB, 0); ex.sim_button(BTN.FB, 0); run(40); };
const MODE = 0, OV = 1, SOLO = 2, KB = 3, FB = 4, SLOT = 5, ACT = 6, EL = 7, PAT = 8, CAT = 9, IDX = 10, PS = 11, PAGE = 12, OCT = 13, TEMPO = 14;
const key = (k, on) => { ex.sim_velocity(100); ex.sim_key(5 + k, on); run(30); };   // k: 0 = F3 (note 53); note 60 = k 7
const voices = (p) => ex.sim_part(p, 5);

check("init", ex.sim_init(44100) === 0);
run(300);

// ---- PATCH
check("boots in PATCH", ex.sim_ui(MODE) === 0);
check("favorite 1 loaded at boot", ex.sim_ui(PAT) === ex.sim_fav(0) && ex.sim_fav(0) > 0, "id " + ex.sim_ui(PAT));
check("header shows bank A", row(0).startsWith("[PATCH] Bank A"), row(0));
tap(2);
check("pad tap loads a favorite", ex.sim_ui(PAT) === ex.sim_fav(2), row(0));
check("header shows the pad", row(0) === "[PATCH] Bank A - 3", row(0));
press(BTN.KB);
check("KNOB BANK in PATCH cycles the bank", ex.sim_ui(KB) === 1 && row(0).includes("Bank B"));
for (let i = 0; i < 3; i++) press(BTN.KB);
check("bank wraps D -> A", ex.sim_ui(KB) === 0);
check("PATCH shows a hint line", row(8).includes("K6 cat"), row(8));
ex.sim_load_patch(449); run(60);
check("an ICONIC name is shown whole", row(1).endsWith("Rhapsody Piano"), row(1));
tap(1);
const sel = ex.sim_ui(PAT);
ex.sim_pad(5, 100); run(800);
check("hold a pad -> overwrite prompt", ex.sim_ui(OV) === 1, row(1));
check("the prompt names the favorite and what replaces it", row(1).startsWith("Overwrite favorite 6") && row(3).startsWith("now:") && row(4).startsWith("with:"), row(3) + " / " + row(4));
ex.sim_pad(5, 0); run(30);
tap(5);
check("same pad confirms the save", ex.sim_fav(5) === sel && ex.sim_ui(OV) === 0 && msg() === "Saved successfully", msg());
run(2100);
check("message disappears", msg() === "");
tap(1);
ex.sim_pad(6, 100); run(800); ex.sim_pad(6, 0); run(5300);
check("prompt timeout cancels without saving", ex.sim_ui(OV) === 0 && ex.sim_fav(6) !== sel);
// categories
enc(5, 1);
check("K6 changes category", ex.sim_ui(CAT) === 2 && ex.sim_ui(IDX) === 0);
const first = ex.sim_ui(PAT);
enc(6, 1);
check("K7 next patch in category or wraps", ex.sim_ui(PAT) > 0, "id " + ex.sim_ui(PAT) + " (cat 2 has " + first + ")");
enc(5, 1);
check("category 3 first patch id 33", ex.sim_ui(PAT) === 33, "id " + ex.sim_ui(PAT));
const cut0 = ex.sim_part(0, 6);
enc(0, 5);
check("K1 tweaks the patch cutoff", ex.sim_part(0, 6) === Math.min(127, cut0 + 10), "" + ex.sim_part(0, 6));
enc(4, 5);
check("K5 tempo", ex.sim_ui(TEMPO) === 125);
tap(0);
check("loading a patch resets the tweak", ex.sim_ui(PAT) === ex.sim_fav(0));

// ---- faders: soft takeover
tap(7);
const dly = ex.sim_part(0, 2);
if (dly > 6 && dly < 110) {
  ex.sim_fader(1, 2); run(20);
  check("fader far from the value does nothing", ex.sim_part(0, 2) === dly, "" + ex.sim_part(0, 2));
  ex.sim_fader(1, Math.min(127, dly + 10)); run(20);
  check("fader crossing the value takes over", ex.sim_part(0, 2) === Math.min(127, dly + 10), "" + ex.sim_part(0, 2));
} else check("fader test skipped (send at " + dly + ")", true);
ex.sim_fader(2, 90); run(20);
check("DIST send follows (0 matched)", ex.sim_part(0, 4) === 90, "" + ex.sim_part(0, 4));

// ---- PERFORM
press(BTN.FB);
check("FADER BANK enters PERFORM and loads A-1", ex.sim_ui(MODE) === 1 && ex.sim_ui(SLOT) === 0 && ex.sim_ui(5) === 0);
check("lines loaded", ex.sim_line(0, 0) === ex.sim_perf(0, 0) && ex.sim_line(3, 0) > 0);
check("header", row(0) === "[PERFORM] Bank A - 1", row(0));
check("L1 range C1-C3", ex.sim_line(0, 1) === 1 && ex.sim_line(0, 2) === 3 && ex.sim_line(1, 1) === 4);
key(0, 1);   // F3 = note 53 -> octave 3
run(300);
check("PERFORM shows level bars on the sounding line", row(3)[1] !== " " && row(4)[1] === " ", row(3) + " | " + row(4));
check("note in C1-C3 goes only to LAYER 1", voices(0) >= 1 && voices(1) === 0 && voices(2) === 0, `${voices(0)}${voices(1)}${voices(2)}`);
key(0, 0); run(1500);
key(7, 1);   // note 60 -> octave 4
check("note in C4-C7 goes only to LAYER 2", voices(1) >= 1 && voices(0) === 0, `${voices(0)}${voices(1)}`);
key(7, 0); run(1500);
press(BTN.OCTU);
check("OCTAVE + shifts the routing", ex.sim_ui(OCT) === 1);
key(0, 1);   // 53 + 12 = 65, octave 4 -> layer 2
check("shifted note goes to LAYER 2", voices(1) >= 1 && voices(0) === 0);
key(0, 0); run(1500);
press(BTN.OCTD); press(BTN.OCTD); press(BTN.OCTD);
check("OCTAVE - clamps at -2", ex.sim_ui(OCT) === -2);
ex.sim_button(BTN.OCTD, 1); ex.sim_button(BTN.OCTU, 1); run(20); ex.sim_button(BTN.OCTD, 0); ex.sim_button(BTN.OCTU, 0); run(30);
check("both OCTAVE buttons reset", ex.sim_ui(OCT) === 0);
tap(7);
check("pad 8 = SOLO 1", ex.sim_ui(SOLO) === 1 && ex.sim_ui(ACT) === 2);
{
  const lit = (p) => ex.sim_pad_rgb(p) !== 0, seen = (p) => { const v = new Set(); for (let i = 0; i < 8; i++) { v.add(ex.sim_pad_rgb(p)); run(130); } return v.size; };
  check("the active solo pad blinks", seen(7) >= 2);
  check("the other solo pad is lit and steady", lit(15) && seen(15) === 1);
  check("a slot with no performance is dark", [...Array(14).keys()].map((i) => (i < 7 ? i : i + 1)).some((p) => !lit(p)));
}
key(0, 1);
check("solo mutes the layers and plays the solo", voices(2) >= 1 && voices(0) === 0 && voices(1) === 0);
key(0, 0); run(1500);
tap(15);
check("pad 16 switches directly to SOLO 2", ex.sim_ui(SOLO) === 2);
tap(2);
check("a combo pad only exits the solo", ex.sim_ui(SOLO) === 0 && ex.sim_ui(5) === 0);
check("... and says so", msg().startsWith("Solo off"), msg());
tap(8);
check("pad 9 = slot 8 selected (empty keeps the performance)", ex.sim_ui(PS) === 7 && ex.sim_ui(5) === 0, "slot " + ex.sim_ui(PS));
tap(1);
check("pad 2 loads A-2", ex.sim_ui(5) === 1 && ex.sim_line(0, 0) === ex.sim_perf(1, 0));
enc(5, 1);
check("K6 changes bank (empty bank keeps the performance)", ex.sim_ui(FB) === 1 && ex.sim_ui(5) === 1);
enc(5, -1);
press(BTN.FB);
check("FADER BANK again cycles banks", ex.sim_ui(FB) === 1);
press(BTN.FB); press(BTN.FB); press(BTN.FB); press(BTN.FB); press(BTN.FB); press(BTN.FB); press(BTN.FB);
check("bank wraps H -> A", ex.sim_ui(FB) === 0);

// ---- EDIT via the chord
tap(0);
chord();
check("chord enters EDIT with no exit prompt", ex.sim_ui(MODE) === 2 && ex.sim_ui(OV) === 0, `mode ${ex.sim_ui(MODE)} ov ${ex.sim_ui(OV)}`);
const l1 = ex.sim_line(1, 0);
enc(0, 1);
check("K1 selects the line", ex.sim_ui(EL) === 1);
enc(2, 1);
check("K3 low octave", ex.sim_line(1, 1) === 5, "" + ex.sim_line(1, 1));
enc(3, -4);
check("K4 high octave pushes the low one", ex.sim_line(1, 2) === 3 && ex.sim_line(1, 1) === 3, `${ex.sim_line(1, 1)}-${ex.sim_line(1, 2)}`);
enc(4, -10);
check("K5 per-line volume", ex.sim_line(1, 3) === 89 && ex.sim_part(1, 0) === Math.floor(89 * 100 / 99), `${ex.sim_line(1, 3)} lvl ${ex.sim_part(1, 0)}`);
enc(1, 1);
check("K2 changes the patch", ex.sim_line(1, 0) !== l1, "" + ex.sim_line(1, 0));
tap(3);
check("pad 4 picks the first Organ", ex.sim_line(1, 0) === 65, "" + ex.sim_line(1, 0));
press(BTN.EDIT);
check("PARA toggles page 2", ex.sim_ui(PAGE) === 1 && row(0).includes("2/2"));
enc(4, 1);
check("K5 toggles sustain", (ex.sim_line(1, 4) & 1) === 0);
enc(6, 2);
check("K7 mod action", (ex.sim_line(1, 4) >> 2) === 2, "" + (ex.sim_line(1, 4) >> 2));
press(BTN.EDIT);
press(BTN.LEFT);
check("arrow opens the search popup", ex.sim_ui(OV) === 3);
enc(0, 1);
tap(0);
check("popup confirm applies the patch", ex.sim_ui(OV) === 0 && ex.sim_line(1, 0) !== 97, "" + ex.sim_line(1, 0));
press(BTN.KB);
check("KNOB BANK in EDIT asks to save", ex.sim_ui(OV) === 4);
tap(1);
check("discard restores the performance", ex.sim_ui(MODE) === 1 && ex.sim_line(1, 0) === ex.sim_perf(0, 1) && ex.sim_line(1, 1) === 4);
chord();
enc(0, 1);
tap(4);
const newp = ex.sim_line(1, 0);
press(BTN.FB);
tap(0);
check("save from the exit prompt stores in the slot", ex.sim_perf(0, 1) === newp && ex.sim_ui(MODE) === 1, `${ex.sim_perf(0, 1)} vs ${newp}`);

// ---- BT save/copy dialog
press(BTN.SAVE);
check("BT opens the dialog", ex.sim_ui(OV) === 2);
enc(5, 1); enc(6, 2);
tap(0);
check("copy to B-3 (index 16)", ex.sim_perf(16, 1) === newp && ex.sim_ui(5) === 16 && msg() === "Saved successfully", `${ex.sim_perf(16, 1)} slot ${ex.sim_ui(5)}`);
press(BTN.SAVE); run(10200);
check("dialog times out", ex.sim_ui(OV) === 0);
press(BTN.KB);
check("KNOB BANK returns to PATCH", ex.sim_ui(MODE) === 0);
check("storage dirty flag raised", ex.sim_db_dirty() === 1);

// ---- pedal, wheels and the per-line sustain / bend / mod
press(BTN.FB);
check("back in PERFORM", ex.sim_ui(MODE) === 1);
ex.sim_pedal(1000); run(60);
check("pedal holds every line with sustain YES", ex.sim_part(0, 7) === 1 && ex.sim_part(1, 7) === 1);
ex.sim_pedal(0); run(60);
check("pedal released", ex.sim_part(0, 7) === 0);
chord();
enc(0, -3);                                  // line 1 (L1)
press(BTN.EDIT);
enc(4, 1);                                   // sustain NO
enc(5, 1);                                   // bend NO
enc(6, 3);                                   // mod: BRIGHT
ex.sim_pedal(1000); run(60);
check("sustain NO line ignores the pedal, others hold", ex.sim_part(0, 7) === 0 && ex.sim_part(1, 7) === 1);
check("bend NO line ignores the bend wheel", ex.sim_part(0, 8) === 1 && ex.sim_part(1, 8) === 0);
check("a non-default mod action disables the vibrato", ex.sim_part(0, 10) === 1 && ex.sim_part(1, 10) === 0);
ex.sim_wheel(1, 485); run(40);
check("mod wheel opens the BRIGHT line's filter only", ex.sim_part(0, 9) > 100 && ex.sim_part(1, 9) === 0, "" + ex.sim_part(0, 9));
ex.sim_wheel(1, 620); run(40);
check("mod wheel at rest: no offset", ex.sim_part(0, 9) === 0);
ex.sim_pedal(0); run(60);
press(BTN.KB); tap(1);
check("discarding restores the line settings", ex.sim_part(0, 8) === 0 && ex.sim_part(0, 10) === 0);

// ---- drum kits (DRUMS engine)
press(BTN.KB);
check("back in PATCH for the drums", ex.sim_ui(MODE) === 0);
enc(5, -2);                                  // ICONIC is category 16, Drums 15
check("category 15 is Drums; a kit loads and the pads become hits", ex.sim_ui(CAT) === 15 && ex.sim_ui(PAT) === 417 && ex.sim_ui(15) === 1, "id " + ex.sim_ui(PAT));
run(100);
check("part 1 switched to the DRUMS engine", ex.sim_part(0, 11) === 2, "" + ex.sim_part(0, 11));
check("header says drum pads", row(0) === "[PATCH] Drum pads", row(0));
ex.sim_peak();
pad(0); run(200);
check("a pad hit makes the kit sound", ex.sim_part(0, 12) === 1 && ex.sim_peak() > 300, "peak " + ex.sim_peak());
ex.sim_pad(0, 0); run(2000);
const m0 = ex.sim_part(0, 6);
enc(0, 4);
check("K1 is MORPH for a kit", ex.sim_part(0, 6) === m0 + 8 && row(9).startsWith("MORPH"), row(9));
enc(6, 1);
check("K7 next kit, pads still hits", ex.sim_ui(PAT) === 418 && ex.sim_ui(15) === 1);
press(BTN.HOME);
check("PATCH button toggles to favorites", ex.sim_ui(15) === 0 && msg() === "Pads: favorites", msg());
tap(1);
check("a pad tap loads a favorite", ex.sim_ui(PAT) === ex.sim_fav(1) && ex.sim_ui(15) === 0);
run(100);
check("the part is back on the synth engine", ex.sim_part(0, 11) === 0);
press(BTN.HOME);
check("PATCH button on a synth patch says so", msg() === "Not a drum kit", msg());

// ---- MIDI input
tap(0); run(100);
ex.sim_midi(0x90, 69, 100); run(120);
check("MIDI note on plays the current patch", voices(0) >= 1, "" + voices(0));
ex.sim_midi(0xB0, 64, 127); run(30);
check("CC 64 holds the sound (sustain)", ex.sim_part(0, 7) === 1);
ex.sim_midi(0xB0, 64, 0); ex.sim_midi(0x80, 69, 0); run(30);
ex.sim_midi(0x90, 72, 100); run(60);
ex.sim_midi(0xB0, 123, 0); run(400);
check("CC 123 releases everything", voices(0) === 0, "" + voices(0));

// ---- panel check
ex.sim_button(BTN.KB, 1); run(800);
check("KNOB BANK held 0.7 s opens the panel check", ex.sim_ui(OV) === 5 && row(0) === "PANEL CHECK", row(0));
check("it shows the faders and the buttons", row(2).startsWith("Faders") && row(5).startsWith("Buttons"), row(5));
const bank0 = ex.sim_ui(KB);
ex.sim_button(BTN.KB, 0); run(60);
check("releasing leaves it without changing the bank", ex.sim_ui(OV) === 0 && ex.sim_ui(KB) === bank0);

// ---- parameter pages
tap(0); run(50);
press(BTN.ARP);
check("ARP button opens the ARP page", row(0) === "[PATCH] ARP" && ex.sim_ui(MODE) === 0, row(0));
enc(0, 1);
check("K1 changes the arp mode", row(3).includes("UP"), row(3));
press(BTN.ARP);
check("pressing it again closes the page", row(0).startsWith("[PATCH] Bank"), row(0));
press(BTN.FX);
check("FX opens the CHORUS page", row(0).includes("CHORUS"), row(0));
const v0 = row(3);
enc(0, 3);
check("a global effect parameter moves", row(3) !== v0, v0 + " -> " + row(3));
press(BTN.FX); press(BTN.FX);
check("FX steps to REVERB", row(0).includes("REVERB"), row(0));
press(BTN.FX);
check("then the global DRIVE send", row(0).includes("DRIVE") && row(3).includes("TYPE"), row(0));
press(BTN.FX);
check("and leaves", row(0).startsWith("[PATCH] Bank"), row(0));
const fm2 = [...Array(ex.sim_patch_n()).keys()].find((i) => ex.sim_patch_engine(i) === 3);
ex.sim_load_patch(ex.sim_patch_id(fm2)); run(100);
press(BTN.LFO);
check("analog-only pages are refused under another engine", msg() === "Not for this engine" && row(0).startsWith("[PATCH] Bank"), msg());
press(BTN.ENV);
check("the amp envelope page is allowed there", row(0).includes("AMP ENV"), row(0));
press(BTN.ENV); press(BTN.ENV);
tap(0);
process.exit(fail ? 1 : 0);
