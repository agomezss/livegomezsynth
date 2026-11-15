// SPDX-License-Identifier: GPL-3.0-only
// Finds patches that sound noisy instead of tonal: one held A4 per patch, then how periodic the steady part is
// (normalised autocorrelation at the pitch lag) and how much energy sits above 6 kHz. Drums and SFX are skipped.
//   node tests/timbre_scan.mjs [--all]       prints the suspects, worst first
import { readFileSync } from "node:fs";
const wasm = readFileSync(
  new URL("../build/sim-site/lgs.wasm", import.meta.url),
);
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
const cat = JSON.parse(
  readFileSync(new URL("../catalog/catalog.json", import.meta.url), "utf8"),
);
const NAME = Object.fromEntries(cat.patches.map((p) => [p.id, p]));
ex.sim_init(44100);
const grab = (ms) => {
  const o = [];
  for (let i = 0; i < ms; i += 2) {
    ex.sim_render(88);
    o.push(...new Float32Array(ex.memory.buffer, ex.sim_out_l(), 88));
  }
  return o;
};
ex.sim_velocity(100);
grab(300);
const FS = 44100,
  N = 8192;
const fft = (re, im) => {
  // in-place radix-2
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) {
    let b = n >> 1;
    for (; j & b; b >>= 1) j ^= b;
    j ^= b;
    if (i < j) {
      [re[i], re[j]] = [re[j], re[i]];
      [im[i], im[j]] = [im[j], im[i]];
    }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = (-2 * Math.PI) / len,
      wr = Math.cos(ang),
      wi = Math.sin(ang);
    for (let i = 0; i < n; i += len) {
      let cr = 1,
        ci = 0;
      for (let k = 0; k < len / 2; k++) {
        const a = i + k,
          b = a + len / 2,
          tr = re[b] * cr - im[b] * ci,
          ti = re[b] * ci + im[b] * cr;
        re[b] = re[a] - tr;
        im[b] = im[a] - ti;
        re[a] += tr;
        im[a] += ti;
        const t = cr * wr - ci * wi;
        ci = cr * wi + ci * wr;
        cr = t;
      }
    }
  }
};
const rows = [];
for (let i = 0; i < ex.sim_patch_n(); i++) {
  const id = ex.sim_patch_id(i),
    p = NAME[id];
  if (p.cat === 15 || p.cat === 13) continue; // Drums, SFX
  ex.sim_load_patch(id);
  grab(120);
  ex.sim_key(21, 1);
  grab(500);
  const a = grab(400),
    b = grab(200); // the steady part
  ex.sim_key(21, 0);
  grab(1500);
  const x = a.concat(b).slice(0, N);
  let e = 0;
  for (const v of x) e += v * v;
  if (e / x.length < 1e-7) continue; // silent here (slow attack)
  // harmonicity: the best normalised autocorrelation for lags from 40 Hz to 1 kHz, over a 2048 window
  const W = 2048;
  let best = 0;
  for (let lag = 44; lag < 1100; lag++) {
    let s = 0,
      e1 = 0,
      e2 = 0;
    for (let k = 0; k < W; k += 2) {
      s += x[k] * x[k + lag];
      e1 += x[k] * x[k];
      e2 += x[k + lag] * x[k + lag];
    }
    const r = s / Math.sqrt(e1 * e2 + 1e-12);
    if (r > best) best = r;
  }
  const re = Float64Array.from(
      x.map((v, k) => v * (0.5 - 0.5 * Math.cos((2 * Math.PI * k) / N))),
    ),
    im = new Float64Array(N);
  fft(re, im);
  let tot = 0,
    hf = 0;
  for (let k = 1; k < N / 2; k++) {
    const m = re[k] * re[k] + im[k] * im[k];
    tot += m;
    if ((k * FS) / N > 6000) hf += m;
  }
  rows.push({
    id,
    name: p.name,
    engine: p.engine,
    cat: p.cat,
    harm: best,
    hf: hf / tot,
  });
}
const bad = rows
  .filter((r) => r.harm < 0.8 || r.hf > 0.15)
  .sort((p, q) => p.harm - q.harm);
if (process.argv.includes("--all"))
  rows.forEach((r) =>
    console.log(r.id, r.engine, r.name, r.harm.toFixed(2), r.hf.toFixed(3)),
  );
console.log(
  `${rows.length} tonal candidates, ${bad.length} suspects (harmonicity < 0.80 or > 15 % above 6 kHz)`,
);
for (const r of bad)
  console.log(
    String(r.id).padEnd(4),
    r.engine.padEnd(4),
    r.name.padEnd(18),
    "harm",
    r.harm.toFixed(2),
    "hf",
    r.hf.toFixed(3),
  );
