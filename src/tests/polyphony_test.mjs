// SPDX-License-Identifier: GPL-3.0-only
// Overlapping notes: drum pads never cut each other (every lane keeps ringing), one-shot effects are polyphonic, leads and
// basses stay monophonic on purpose.
//   sh web/sim/build.sh && node tests/polyphony_test.mjs
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const CAT = JSON.parse(readFileSync(new URL("../catalog/catalog.json", import.meta.url), "utf8")).patches;
const idOf = (name) => CAT.find((x) => x.name === name).id;
let fail = 0;
const check = (n, ok, i = "") => { console.log((ok ? "  ok    " : "  FAIL  ") + n + (i !== "" ? "  (" + i + ")" : "")); if (!ok) fail++; };
const boot = async () => { const { instance } = await WebAssembly.instantiate(wasm, {}); const ex = instance.exports; ex.sim_init(44100); return ex; };
const grab = (ex, ms) => { const o = []; for (let i = 0; i < ms; i += 2) { ex.sim_render(88); o.push(...new Float32Array(ex.memory.buffer, ex.sim_out_l(), 88)); } return o; };
const rms = (x) => Math.sqrt(x.reduce((s, v) => s + v * v, 0) / Math.max(1, x.length));

// drum kit: a hit on pad 0 (kick), then another pad while it rings. The mix must be the sum of each hit played alone
// (nothing cut, nothing choked); the master effects add a little, so allow 5 %.
async function drums(id, first, second, hold) {
  const ex = await boot(); grab(ex, 300); ex.sim_load_patch(id); grab(ex, 200);
  const out = [];
  if (first) ex.sim_pad(0, 100);
  out.push(...grab(ex, 30)); if (first && !hold) ex.sim_pad(0, 0);
  out.push(...grab(ex, 100));
  if (second !== null) ex.sim_pad(second, 100);
  out.push(...grab(ex, 30)); if (second !== null) ex.sim_pad(second, 0); if (first && hold) ex.sim_pad(0, 0);
  out.push(...grab(ex, 700));
  return out;
}
for (const id of ["DRM Deep Boom", "DRM Club Kit", "DRM Drum-X"].map(idOf)) for (const hold of [false, true]) {
  const kick = await drums(id, true, null, hold);
  let worst = 0;
  for (const s of [1, 2, 3, 5, 6, 7]) {
    const sec = await drums(id, false, s, hold), both = await drums(id, true, s, hold);
    const sum = kick.map((v, i) => v + sec[i]);
    worst = Math.max(worst, rms(both.map((v, i) => v - sum[i])) / rms(sum));
  }
  check(`kit ${id}: a second pad${hold ? " (first still held)" : ""} cuts nothing`, worst < 0.05, worst.toFixed(3));
}

// a drum pad hits at a fixed velocity (a soft touch is not quieter), and a hit within 40 ms of the same pad's release is a rebound: ignored
{
  const hit = async (vel, gap, vel2) => {
    const ex = await boot(); grab(ex, 300); ex.sim_load_patch(idOf("DRM Deep Boom")); grab(ex, 200);
    ex.sim_pad(0, vel); const o = grab(ex, 30); ex.sim_pad(0, 0);
    o.push(...grab(ex, gap));
    if (vel2) { ex.sim_pad(0, vel2); o.push(...grab(ex, 30)); ex.sim_pad(0, 0); } else o.push(...grab(ex, 30));
    o.push(...grab(ex, 700));
    return o;
  };
  const soft = await hit(8, 100, 0), hard = await hit(127, 100, 0);
  check("a soft pad touch hits as hard as a firm one", Math.abs(rms(soft) / rms(hard) - 1) < 0.01, (rms(soft) / rms(hard)).toFixed(3));
  const one = await hit(100, 10, 0), reb = await hit(100, 10, 30), two = await hit(100, 80, 30);
  check("a hit 10 ms after the release is ignored", Math.abs(rms(reb) / rms(one) - 1) < 0.01, (rms(reb) / rms(one)).toFixed(3));
  check("a hit 80 ms after the release plays", Math.abs(rms(two) / rms(one) - 1) > 0.03, (rms(two) / rms(one)).toFixed(3));
}

// polyphony of one-shot effects and the monophonic leads
const ex = await boot(); grab(ex, 300);
const voices = (id) => { ex.sim_load_patch(id); grab(ex, 100); for (const k of [12, 16, 19]) ex.sim_key(k, 1); grab(ex, 150); const v = ex.sim_voices(); for (const k of [12, 16, 19]) ex.sim_key(k, 0); grab(ex, 1500); return v; };
for (const [id, name] of [[idOf("SYN Laser"), "Laser"]]) check(`${name} plays overlapping hits`, voices(id) === 3, "" + voices(id));
check("a bass stays monophonic on purpose (SYN Fat Bass)", voices(idOf("SYN Fat Bass")) === 1);
check("so does a lead (SYN Sync Lead)", voices(idOf("SYN Sync Lead")) === 1);
process.exit(fail ? 1 : 0);
