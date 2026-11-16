// SPDX-License-Identifier: GPL-3.0-only
// Storage image (T1.9): a saved favorite survives a restart, a bad CRC / wrong size falls back to the defaults,
// a stale patch id is emptied by the sanitizer.
//   sh web/sim/build.sh && node tests/store_test.mjs
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
let fail = 0;
const check = (n, ok, i = "") => { console.log((ok ? "  ok    " : "  FAIL  ") + n + (i !== "" ? "  (" + i + ")" : "")); if (!ok) fail++; };
const boot = async (img) => {
  const { instance } = await WebAssembly.instantiate(wasm, {});
  const ex = instance.exports;
  if (img) { new Uint8Array(ex.memory.buffer, ex.sim_stage(), img.length).set(img); ex.sim_stage_set(img.length); }
  ex.sim_init(44100);                        // the start-up reads the staged image
  for (let i = 0; i < 300; i += 2) ex.sim_render(88);
  return ex;
};
const run = (ex, ms) => { for (let i = 0; i < ms; i += 2) ex.sim_render(88); };
const tap = (ex, p) => { ex.sim_pad(p, 100); run(ex, 30); ex.sim_pad(p, 0); run(ex, 30); };

let a = await boot();
check("defaults at first boot", a.sim_fav(0) > 0 && a.sim_fav(15) === 0);
const def9 = a.sim_fav(9);
tap(a, 2);
a.sim_pad(9, 100); run(a, 800); a.sim_pad(9, 0); run(a, 30); tap(a, 9);
const saved = a.sim_fav(9);
check("favorite saved in RAM", saved === a.sim_fav(2) && saved !== def9);
check("dirty flag raised", a.sim_db_dirty() === 1 && a.sim_db_dirty() === 0);
const n = a.sim_db_size();
const img = new Uint8Array(a.memory.buffer, a.sim_db_export(), n).slice();

let b = await boot(img);
check("image restored after restart", b.sim_fav(9) === saved, "" + b.sim_fav(9));
check("restored image is not dirty", b.sim_db_dirty() === 0);

const bad = img.slice(); bad[100] ^= 0xff;
let c = await boot(bad);
check("bad CRC -> defaults", c.sim_fav(9) === def9);
let d = await boot(img.slice(0, n - 4));
check("wrong size -> defaults", d.sim_fav(9) === def9);

const stale = img.slice();
const dv = new DataView(stale.buffer);
dv.setUint16(16 + 9 * 10, 9999, true);       // fav[9].patch = an id that is not in the catalog
const sum = (() => { let cr = 0xffffffff; for (let i = 0; i < n - 4; i++) { cr ^= stale[i]; for (let k = 0; k < 8; k++) cr = cr & 1 ? (cr >>> 1) ^ 0xedb88320 : cr >>> 1; } return (~cr) >>> 0; })();
dv.setUint32(n - 4, sum, true);
let e = await boot(stale);
check("stale patch id is emptied by the sanitizer", e.sim_fav(9) === 0 && e.sim_fav(0) > 0, "" + e.sim_fav(9));

// an image written with the first numbering of the patch ids (up to 0.1.12) is mapped once to the current one
{
  const old = img.slice(), dv2 = new DataView(old.buffer);
  const crc = () => { let cr = 0xffffffff; for (let i = 0; i < n - 4; i++) { cr ^= old[i]; for (let k = 0; k < 8; k++) cr = cr & 1 ? (cr >>> 1) ^ 0xedb88320 : cr >>> 1; } dv2.setUint32(n - 4, (~cr) >>> 0, true); };
  dv2.setUint32(12, dv2.getUint32(12, true) & 0xff, true);       // db.theme: the generation byte back to 0
  dv2.setUint16(16 + 0 * 10, 451, true);                          // Classic Piano (first numbering)
  dv2.setUint16(16 + 1 * 10, 449, true);                          // Rhapsody Piano: removed
  dv2.setUint16(16 + 2 * 10, 1, true);                            // SYN Tape Keys
  dv2.setUint16(16 + 64 * 10, 417, true);                         // performance 1, line 1: DRM Deep Boom
  crc();
  const m = await boot(old);
  check("old numbering: a kept patch gets its new number", m.sim_fav(0) === 22 && m.sim_fav(2) === 38, `${m.sim_fav(0)} ${m.sim_fav(2)}`);
  check("old numbering: a removed patch leaves its slot empty", m.sim_fav(1) === 0, "" + m.sim_fav(1));
  check("old numbering: performance lines are mapped too", m.sim_perf(0, 0) === 345, "" + m.sim_perf(0, 0));
  check("old numbering: the image is marked for saving", m.sim_db_dirty() === 1);
  const again = new Uint8Array(m.memory.buffer, m.sim_db_export(), n).slice();
  const m2 = await boot(again);
  check("and is not mapped a second time", m2.sim_fav(0) === 22 && m2.sim_fav(2) === 38 && m2.sim_db_dirty() === 0, `${m2.sim_fav(0)} ${m2.sim_fav(2)}`);
}

// an image of the second numbering (0.1.13) is mapped once to the current one
{
  const old = img.slice(), dv2 = new DataView(old.buffer);
  const crc = () => { let cr = 0xffffffff; for (let i = 0; i < n - 4; i++) { cr ^= old[i]; for (let k = 0; k < 8; k++) cr = cr & 1 ? (cr >>> 1) ^ 0xedb88320 : cr >>> 1; } dv2.setUint32(n - 4, (~cr) >>> 0, true); };
  dv2.setUint32(12, (dv2.getUint32(12, true) & 0xff) | (1 << 8), true);   // db.theme: generation 1
  dv2.setUint16(16 + 0 * 10, 22, true);                           // Classic Piano
  dv2.setUint16(16 + 2 * 10, 30, true);                           // first E-Piano
  dv2.setUint16(16 + 64 * 10, 211, true);                         // performance 1, line 1: DRM Deep Boom
  crc();
  const m = await boot(old);
  check("second numbering: patches get their new numbers", m.sim_fav(0) === 22 && m.sim_fav(2) === 38 && m.sim_perf(0, 0) === 345, `${m.sim_fav(0)} ${m.sim_fav(2)} ${m.sim_perf(0, 0)}`);
  check("second numbering: the image is marked for saving", m.sim_db_dirty() === 1);
}

// the screen theme (SEQ button) survives a restart
const press = (ex, b) => { ex.sim_button(b, 1); run(ex, 20); ex.sim_button(b, 0); run(ex, 3000); };   // 3 s: the "saved" message is gone
let t0 = await boot();
const classic = t0.sim_fb_hash();
press(t0, 15);
const studio = t0.sim_fb_hash();
check("SEQ switches the screen theme", studio !== classic);
check("and lights its LED", (t0.sim_leds_buttons() >>> 15 & 1) === 1);
const timg = new Uint8Array(t0.memory.buffer, t0.sim_db_export(), t0.sim_db_size()).slice();
let t1 = await boot(timg);
check("the theme is back after a restart", t1.sim_fb_hash() === studio && (t1.sim_leds_buttons() >>> 15 & 1) === 1);
const seen = new Set([classic, studio]);
for (let i = 2; i < 8; i++) { press(t1, 15); seen.add(t1.sim_fb_hash()); }
check("SEQ steps through eight different themes", seen.size === 8, "" + seen.size);
press(t1, 15);
check("and the eighth press returns to the classic theme", t1.sim_fb_hash() === classic);
process.exit(fail ? 1 : 0);
