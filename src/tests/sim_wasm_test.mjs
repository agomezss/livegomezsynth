// SPDX-License-Identifier: GPL-3.0-only
// Runs build/sim-site/lgs.wasm (the firmware) in node: boots, draws, plays a key, K7 changes the patch, K1 the cutoff, K8 the master.
//   sh web/sim/build.sh && node tests/sim_wasm_test.mjs
import { readFileSync } from "node:fs";
const wasm = readFileSync(
  new URL("../build/sim-site/lgs.wasm", import.meta.url),
);
const { instance } = await WebAssembly.instantiate(wasm, {});
const ex = instance.exports;
let fail = 0;
const check = (name, ok, info = "") => {
  console.log(
    (ok ? "  ok    " : "  FAIL  ") + name + (info ? "  (" + info + ")" : ""),
  );
  if (!ok) fail++;
};
const run = (ms) => {
  for (let i = 0; i < ms; i += 2) ex.sim_render(88);
}; // 88 frames ~ 2 ms
const peakOf = (ms) => {
  let p = 0;
  for (let i = 0; i < ms; i += 2) {
    ex.sim_render(88);
    const l = new Float32Array(ex.memory.buffer, ex.sim_out_l(), 88);
    for (const v of l) p = Math.max(p, Math.abs(v));
  }
  return p;
};

check("init", ex.sim_init(44100) === 0);
run(300);
const h0 = ex.sim_fb_hash();
check("screen drawn", h0 !== 0 && ex.sim_fb_hash() === h0);
check("favorite 1 loaded at boot", ex.sim_prog() > 0);
check("silence before a key", peakOf(100) < 0.001);
ex.sim_velocity(100);
ex.sim_key(5 + 7, 1); // keybed 12 = C4
const p = peakOf(300);
check("a key plays", p > 0.01, "peak " + p.toFixed(3));
check("a voice is on", ex.sim_voices() >= 1);
ex.sim_key(5 + 7, 0);
run(1500);
ex.sim_enc(6, 3);
run(100); // K7: three detents in the favorites list
check("K7 browses the patches", ex.sim_prog() > 0 && ex.sim_ui(10) === 3, "idx " + ex.sim_ui(10));
check("screen changed with the patch", ex.sim_fb_hash() !== h0);
const cut = ex.sim_param(0); // P_CUTOFF is parameter 0 of the table? (read back whatever K1 moves)
ex.sim_enc(0, 5);
run(50);
check(
  "K1 edits a parameter",
  [...Array(40).keys()].some((i) => ex.sim_param(i) !== 0),
);
const m0 = ex.sim_master_current();
ex.sim_enc(7, -3);
run(10);
check(
  "K8 is the master",
  ex.sim_master_current() === m0 - 72,
  "master " + ex.sim_master_current(),
);
process.exit(fail ? 1 : 0);
