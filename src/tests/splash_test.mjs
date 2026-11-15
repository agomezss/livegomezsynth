// SPDX-License-Identifier: GPL-3.0-only
// Start-up splash: shown for 5 s, ends by itself or on the first button, the normal screen is intact underneath.
//   sh web/sim/build.sh && node tests/splash_test.mjs
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
let fail = 0;
const check = (n, ok, i = "") => { console.log((ok ? "  ok    " : "  FAIL  ") + n + (i !== "" ? "  (" + i + ")" : "")); if (!ok) fail++; };
const boot = async (ms) => {
  const { instance } = await WebAssembly.instantiate(wasm, {});
  const ex = instance.exports;
  if (ms !== undefined) ex.sim_splash_set(ms);
  ex.sim_init(44100);
  return ex;
};
const run = (ex, ms) => { for (let i = 0; i < ms; i += 2) ex.sim_render(88); };
const str = (ex, p) => { const m = new Uint8Array(ex.memory.buffer); let s = ""; while (m[p]) s += String.fromCharCode(m[p++]); return s; };

const plain = await boot();
run(plain, 300);
const normal = plain.sim_fb_hash();

const a = await boot(5000);
run(a, 300);
const early = a.sim_fb_hash();
check("the splash replaces the screen at start-up", early !== normal);
check("the normal screen text is already prepared underneath", str(a, a.sim_row(0)).startsWith("[PATCH]"), str(a, a.sim_row(0)));
run(a, 4400);
check("still there after 4.7 s", a.sim_fb_hash() === early);
run(a, 600);
check("gone after 5 s", a.sim_fb_hash() === normal);

const b = await boot(5000);
run(b, 500);
b.sim_button(5, 1); run(b, 20); b.sim_button(5, 0); run(b, 100);
check("a button ends it early", b.sim_fb_hash() !== early);

const c = await boot(5000);
run(c, 500);
c.sim_pad(2, 100); run(c, 20); c.sim_pad(2, 0); run(c, 100);
check("so does a pad", c.sim_fb_hash() !== early);
process.exit(fail ? 1 : 0);
