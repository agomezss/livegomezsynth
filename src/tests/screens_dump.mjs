// SPDX-License-Identifier: GPL-3.0-only
// Dumps the panel screen of each UI state as PNG into build/shots and compares their hashes with tests/golden-screens.json.
//   node tests/screens_dump.mjs [--update]   (--update rewrites the goldens after an intended screen change)
import { readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { deflateSync } from "node:zlib";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
const run = (ms) => { for (let i = 0; i < ms; i += 2) ex.sim_render(88); };
const out = new URL("../build/shots/", import.meta.url);
mkdirSync(out, { recursive: true });
const shot = (name) => {
  run(60);
  const px = new Uint16Array(ex.memory.buffer, ex.sim_fb(), 240 * 240);
  const b = Buffer.alloc(240 * 240 * 3);
  for (let i = 0; i < px.length; i++) {
    const v = px[i];
    b[i * 3] = ((v >> 11) & 31) * 255 / 31; b[i * 3 + 1] = ((v >> 5) & 63) * 255 / 63; b[i * 3 + 2] = (v & 31) * 255 / 31;
  }
  shots.push(b);
  hashes[name] = fnv(b);
  writeFileSync(new URL(name + ".png", out), png(240, 240, b));
};
const shots = [], hashes = {};
const fnv = (b) => { let h = 2166136261; for (const x of b) h = Math.imul(h ^ x, 16777619) >>> 0; return h.toString(16); };
const crcT = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
const crc = (buf) => { let c = 0xffffffff; for (const x of buf) c = crcT[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
const png = (w, h, rgb) => {
  const raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++) { raw[y * (w * 3 + 1)] = 0; rgb.copy(raw, y * (w * 3 + 1) + 1, y * w * 3, (y + 1) * w * 3); }
  const ih = Buffer.alloc(13); ih.writeUInt32BE(w, 0); ih.writeUInt32BE(h, 4); ih[8] = 8; ih[9] = 2;
  return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ih), chunk("IDAT", deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]);
};
const press = (b) => { ex.sim_button(b, 1); run(20); ex.sim_button(b, 0); run(30); };
const tap = (p) => { ex.sim_pad(p, 100); run(30); ex.sim_pad(p, 0); run(30); };
const enc = (e, n) => { ex.sim_enc(e, n); run(20); };
ex.sim_init(44100); run(300);
tap(7); enc(5, 2); enc(6, 2);
shot("1-patch");
ex.sim_fader(1, 40); run(20); shot("1b-patch-fader");
press(1); shot("2-perform");
tap(7); shot("2b-perform-solo");
tap(7);
ex.sim_button(0, 1); run(40); ex.sim_button(1, 1); run(40); ex.sim_button(0, 0); ex.sim_button(1, 0); run(40);
shot("3-edit-p1");
press(13); shot("3b-edit-p2");
press(13); press(2); enc(0, 1); shot("4-popup");
tap(1);
press(11); enc(5, 1); shot("5-save-dialog");
tap(1);
press(0); shot("6-exit");
tap(1); press(0);
ex.sim_pad(4, 100); run(800); shot("7-fav-save");

const W = 4, H = Math.ceil(shots.length / W), SW = 240 * W + 10 * (W - 1), SH = 240 * H + 10 * (H - 1);
const sheet = Buffer.alloc(SW * SH * 3, 40);
shots.forEach((b, i) => { const ox = (i % W) * 250, oy = Math.floor(i / W) * 250; for (let y = 0; y < 240; y++) b.copy(sheet, ((oy + y) * SW + ox) * 3, y * 720, (y + 1) * 720); });
writeFileSync(new URL("sheet.png", out), png(SW, SH, sheet));

const gold = new URL("golden-screens.json", import.meta.url);
if (process.argv.includes("--update")) {
  writeFileSync(gold, JSON.stringify(hashes, null, 1) + "\n");
  console.log("goldens updated:", Object.keys(hashes).length);
} else {
  let want = {};
  try { want = JSON.parse(readFileSync(gold, "utf8")); } catch (e) {}
  const bad = Object.keys(hashes).filter((k) => want[k] !== hashes[k]);
  bad.forEach((k) => console.log("  FAIL  screen differs from golden: " + k));
  console.log(bad.length ? "screens: " + bad.length + " differ (see build/shots; --update if intended)" : "screens: " + Object.keys(hashes).length + " match the goldens");
  process.exit(bad.length ? 1 : 0);
}
