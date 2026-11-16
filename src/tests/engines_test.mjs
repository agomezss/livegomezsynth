// SPDX-License-Identifier: GPL-3.0-only
// Every catalog patch plays: a note makes sound at a sane level, stops after the release, and the plugged engines
// (4-op FM, drawbar organ) play the right pitch. Prints a level table (peak per patch).
//   sh web/sim/build.sh && node tests/engines_test.mjs [--table]
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
const CAT = JSON.parse(readFileSync(new URL("../catalog/catalog.json", import.meta.url), "utf8")).patches;
const idOf = (name) => CAT.find((x) => x.name === name).id;
let fail = 0;
const check = (n, ok, i = "") => { console.log((ok ? "  ok    " : "  FAIL  ") + n + (i !== "" ? "  (" + i + ")" : "")); if (!ok) fail++; };
ex.sim_init(44100);
const grab = (ms) => {                       // render ms of audio, return the left samples
  const out = [];
  for (let i = 0; i < ms; i += 2) {
    ex.sim_render(88);
    out.push(...new Float32Array(ex.memory.buffer, ex.sim_out_l(), 88));
  }
  return out;
};
const peak = (a) => a.reduce((m, v) => Math.max(m, Math.abs(v)), 0);
const freq = (a) => {                         // zero-crossing estimate over the middle of the buffer (Hz)
  const b = a.slice(Math.floor(a.length / 4), Math.floor((3 * a.length) / 4));
  let z = 0;
  for (let i = 1; i < b.length; i++) if (b[i - 1] < 0 && b[i] >= 0) z++;
  return (z * 44100) / b.length;
};
ex.sim_velocity(100);
grab(300);
const ENG = ["SYN", "?", "DRM", "FM2", "ORG", "SP", "FM6", "VOC"];
const rows = [];
let silent = 0, hot = 0, stuck = 0;
for (let i = 0; i < ex.sim_patch_n(); i++) {
  const id = ex.sim_patch_id(i), eng = ex.sim_patch_engine(i);
  ex.sim_load_patch(id);
  grab(120);
  let p;
  if (eng === 2) { ex.sim_pad(0, 100); const a = grab(300); ex.sim_pad(0, 0); p = peak(a); }
  else { ex.sim_key(5 + 16, 1); const a = grab(1500); ex.sim_key(5 + 16, 0); p = peak(a); }   // A4 (note 69)
  let t = 0, tail = 1;                        // how long until the sound is gone (max 12 s)
  while (t < 12000 && (tail = peak(grab(250))) > 0.003) t += 250;
  rows.push([id, ENG[eng], p.toFixed(3), (t / 1000).toFixed(2) + " s"]);
  if (p < 0.01) silent++;
  if (p > 0.98) hot++;
  if (tail > 0.003) stuck++;
}
if (process.argv.includes("--table")) rows.forEach((r) => console.log(r.join("  ")));
check("every patch makes sound", silent === 0, silent + " silent");
check("no patch clips at one note", hot === 0, hot + " above 0.98");
check("every patch is silent within 12 s of its release", stuck === 0, stuck + " still sounding");
ex.sim_load_patch(ex.sim_patch_id(0));
const flute = [...Array(ex.sim_patch_n()).keys()].map((i) => [ex.sim_patch_id(i), ex.sim_patch_engine(i)]).filter(([, e]) => e === 4);
check("catalog has the organ patches", flute.length === 24, "" + flute.length);
// ORG "Soft Flute": only the 8' bar -> a sine at the note
const softFlute = [...Array(ex.sim_patch_n()).keys()].find((i) => ex.sim_patch_engine(i) === 4 && ex.sim_patch_id(i) === idOf("ORG Soft Flute"));
if (softFlute !== undefined) {
  ex.sim_load_patch(idOf("ORG Soft Flute")); grab(120);
  ex.sim_key(5 + 16, 1); const a = grab(600); ex.sim_key(5 + 16, 0);
  const f = freq(a);
  check("ORG Soft Flute plays A4 at 440 Hz", Math.abs(f - 440) < 6, f.toFixed(1) + " Hz");
}
const domFreq = (a, lo, hi) => {              // strongest frequency in [lo, hi] Hz by a plain DFT
  let best = 0, bf = 0;
  for (let f = lo; f <= hi; f += 1) {
    let re = 0, im = 0;
    for (let n = 0; n < a.length; n += 2) { const w = (2 * Math.PI * f * n) / 44100; re += a[n] * Math.cos(w); im += a[n] * Math.sin(w); }
    const m = Math.hypot(re, im);
    if (m > best) { best = m; bf = f; }
  }
  return bf;
};
for (const [id, name] of [["SP Grand Piano"], ["Flute Solo"], ["FM6 Drawbars"]].map(([n]) => [idOf(n), n])) {
  ex.sim_load_patch(id); grab(120);
  ex.sim_key(5 + 16, 1); grab(300); const a = grab(300); ex.sim_key(5 + 16, 0); grab(2000);
  const f = domFreq(a, 400, 480);
  check(name + " plays A4 near 440 Hz", Math.abs(f - 440) < 6, f + " Hz");
}
process.exit(fail ? 1 : 0);
