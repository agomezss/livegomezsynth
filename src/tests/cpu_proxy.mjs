// SPDX-License-Identifier: GPL-3.0-only
// Relative render cost of each engine in the WebAssembly build (eight held notes, 3 s of audio): a proxy, not device cycles.
//   node tests/cpu_proxy.mjs                 per engine
//   node tests/cpu_proxy.mjs --patches [N]   every patch with a 3-note chord and a solo note (4 voices), the N costliest (default 25)
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

if (process.argv.includes("--patches")) {
  const n = +(process.argv[process.argv.indexOf("--patches") + 1] || 25) || 25;
  const names = JSON.parse(readFileSync(new URL("../catalog/catalog.json", import.meta.url), "utf8")).patches;
  const nameOf = Object.fromEntries(names.map((p) => [p.id, p.name]));
  const cost = [];
  for (let i = 0; i < ex.sim_patch_n(); i++) {
    const id = ex.sim_patch_id(i), e = ex.sim_patch_engine(i);
    ex.sim_load_patch(id); run(100);
    const keys = e === 2 ? [] : [10, 14, 17, 22];
    for (const k of keys) ex.sim_key(k, 1);
    if (e === 2) ex.sim_pad(0, 100);
    run(150);
    let best = Infinity;
    for (let r = 0; r < 3; r++) { const t0 = performance.now(); run(500); best = Math.min(best, performance.now() - t0); }
    for (const k of keys) ex.sim_key(k, 0);
    if (e === 2) ex.sim_pad(0, 0);
    run(400);
    cost.push({ id, name: nameOf[id], eng: ENG[e], ms: best });
  }
  cost.sort((a, b) => a.ms - b.ms);
  const med = cost[cost.length >> 1].ms;
  console.log("\nmedian patch " + (med / 500 * 100).toFixed(2) + "% of real time (4 voices, host)");
  const by = {};
  for (const c of cost) (by[c.eng] ??= []).push(c.ms / med);
  for (const [k, v] of Object.entries(by)) { v.sort((a, b) => a - b); console.log(k.padEnd(4), "n=" + String(v.length).padStart(3), "median x" + v[v.length >> 1].toFixed(2), "max x" + v[v.length - 1].toFixed(2)); }
  console.log("\ncostliest " + n + ":");
  for (const c of cost.slice(-n).reverse()) console.log("  x" + (c.ms / med).toFixed(2), c.eng.padEnd(4), String(c.id).padStart(3), c.name);
}
