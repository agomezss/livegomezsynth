// SPDX-License-Identifier: GPL-3.0-only
// Relative render cost of each engine in the WebAssembly build (eight held notes, 3 s of audio): a proxy, not device cycles.
//   node tests/cpu_proxy.mjs
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
ex.sim_init(44100);
const run = (ms) => { for (let i = 0; i < ms; i += 2) ex.sim_render(88); };
run(300);
const ENG = ["SYN", "?", "DRM", "FM2", "ORG", "SP", "FM6", "VOC"];
const pick = {};
for (let i = 0; i < ex.sim_patch_n(); i++) { const e = ex.sim_patch_engine(i); if (!(e in pick) && e !== 2) pick[e] = ex.sim_patch_id(i); }
const res = {};
for (const e of Object.keys(pick)) {
  ex.sim_load_patch(pick[e]); run(150);
  for (let k = 0; k < 8; k++) ex.sim_key(5 + k * 3, 1);
  run(200);
  const t0 = performance.now(); run(3000); const dt = performance.now() - t0;
  for (let k = 0; k < 8; k++) ex.sim_key(5 + k * 3, 0);
  run(6000);
  res[ENG[e]] = dt;
}
const base = res.SYN;
for (const [k, v] of Object.entries(res)) console.log(k.padEnd(4), (v / 3000 * 100).toFixed(1) + "% of real time", "  x" + (v / base).toFixed(2) + " vs SYN");
