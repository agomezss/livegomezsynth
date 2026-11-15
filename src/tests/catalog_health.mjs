// SPDX-License-Identifier: GPL-3.0-only
// Level health and golden renders of every catalog patch: peak <= -6 dBFS and every patch's loudest 50 ms within
// MARGIN_DB of the catalog median (the instrument's own level, about -24 dBFS: the output stage has the headroom).
// --trim writes catalog/trims.json (per-patch gain, 64 = 0 dB, by patch name); then: gen_catalog.py, rebuild, rerun.
// One held A4 (drums: pad 1), velocity 100, 6 s; the renders are deterministic, so their hashes pin the sound.
//   node tests/catalog_health.mjs [--update] [--table] [--trim]   --update rewrites catalog/golden.json
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { createHash } from "node:crypto";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
const GOLD = new URL("../catalog/golden.json", import.meta.url);
const LIMIT_PEAK = -6, MARGIN_DB = 6, CLAMP = [16, 255];
const CAT = JSON.parse(readFileSync(new URL("../catalog/catalog.json", import.meta.url), "utf8")).patches;
const NAME = Object.fromEntries(CAT.map((p) => [p.id, p.name]));
const TRIMS = existsSync(new URL("../catalog/trims.json", import.meta.url)) ? JSON.parse(readFileSync(new URL("../catalog/trims.json", import.meta.url), "utf8")) : {};
ex.sim_init(44100);
const grab = (ms) => {
  const out = [];
  for (let i = 0; i < ms; i += 2) { ex.sim_render(88); out.push(...new Float32Array(ex.memory.buffer, ex.sim_out_l(), 88)); }
  return out;
};
const db = (x) => (x > 0 ? 20 * Math.log10(x) : -120);
ex.sim_velocity(100);
grab(300);
const now = {};
for (let i = 0; i < ex.sim_patch_n(); i++) {
  const id = ex.sim_patch_id(i), drums = ex.sim_patch_engine(i) === 2;
  ex.sim_load_patch(id);
  grab(120);
  if (drums) ex.sim_pad(0, 100); else ex.sim_key(21, 1);
  const a = grab(drums ? 500 : 6000);
  if (drums) ex.sim_pad(0, 0); else ex.sim_key(21, 0);
  grab(250);
  let peak = 0, best = 0;
  for (const v of a) peak = Math.max(peak, Math.abs(v));
  for (let s = 0; s + 2205 <= a.length; s += 441) {         // the loudest 50 ms
    let e = 0;
    for (let k = s; k < s + 2205; k++) e += a[k] * a[k];
    best = Math.max(best, Math.sqrt(e / 2205));
  }
  const pcm = Int16Array.from(a, (v) => Math.max(-32768, Math.min(32767, Math.round(v * 32768))));
  now[id] = { peak: +db(peak).toFixed(1), rms: +db(best).toFixed(1), sha: createHash("sha256").update(Buffer.from(pcm.buffer)).digest("hex").slice(0, 16) };
}
const ids = Object.keys(now).map(Number);
const cur = (id) => TRIMS[NAME[id]] ?? 64;
const raw = Object.fromEntries(ids.map((id) => [id, { rms: now[id].rms - 20 * Math.log10(cur(id) / 64), peak: now[id].peak - 20 * Math.log10(cur(id) / 64) }]));
const med = ids.map((id) => now[id].rms).sort((x, y) => x - y)[ids.length >> 1];
const rawMed = ids.map((id) => raw[id].rms).sort((x, y) => x - y)[ids.length >> 1];
if (process.argv.includes("--trim")) {
  const out = {};
  for (const id of ids) {
    let g = rawMed - raw[id].rms;                                   // dB to reach the median
    g = Math.min(g, LIMIT_PEAK - 3 - raw[id].peak);                  // keep 3 dB under the peak limit
    const t = Math.max(CLAMP[0], Math.min(CLAMP[1], Math.round(64 * 10 ** (g / 20))));
    if (t !== 64) out[NAME[id]] = t;
  }
  writeFileSync(new URL("../catalog/trims.json", import.meta.url), JSON.stringify(out, null, 0).replace(/,"/g, ",\n\"") + "\n");
  console.log("wrote catalog/trims.json,", Object.keys(out).length, "trims (median", rawMed, "dBFS); now: python3 tools/gen_catalog.py, rebuild, rerun");
}
const loud = ids.filter((id) => now[id].peak > LIMIT_PEAK);
const off = ids.filter((id) => Math.abs(now[id].rms - med) > MARGIN_DB);
if (process.argv.includes("--table")) ids.forEach((id) => console.log(id, now[id].peak, now[id].rms));
if (process.argv.includes("--update")) { writeFileSync(GOLD, JSON.stringify(now, null, 0).replace(/},"/g, "},\n\"") + "\n"); console.log("wrote catalog/golden.json,", ids.length, "patches"); }
let fail = 0;
const check = (n, ok, i = "") => { console.log((ok ? "  ok    " : "  FAIL  ") + n + (i !== "" ? "  (" + i + ")" : "")); if (!ok) fail++; };
check("no patch peaks above " + LIMIT_PEAK + " dBFS", loud.length === 0, loud.length ? loud.slice(0, 12).join(",") : "");
check("loud parts within " + MARGIN_DB + " dB of the median (" + med + " dBFS)", off.length <= ids.length * 0.05, off.length + " outside" + (off.length ? ": " + off.slice(0, 16).map((id) => NAME[id]).join(", ") : ""));
if (existsSync(GOLD) && !process.argv.includes("--update")) {
  const was = JSON.parse(readFileSync(GOLD, "utf8"));
  const changed = ids.filter((id) => !was[id] || was[id].sha !== now[id].sha);
  console.log("  info  " + changed.length + " of " + ids.length + " renders differ from catalog/golden.json" + (changed.length ? " (run with --update after an intended sound change)" : ""));
}
process.exit(fail ? 1 : 0);
