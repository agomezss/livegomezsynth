// SPDX-License-Identifier: GPL-3.0-only
// Random panel + MIDI input against the firmware in WebAssembly: no trap, no hang, state stays in range, output stays finite.
//   node tests/fuzz.mjs [seeds=6] [steps=4000]
import { readFileSync } from "node:fs";
const wasm = readFileSync(new URL("../build/sim-site/lgs.wasm", import.meta.url));
const seeds = +(process.argv[2] || 6), steps = +(process.argv[3] || 4000);
let fail = 0;
for (let seed = 1; seed <= seeds; seed++) {
  let s = seed * 2654435761 >>> 0;
  const rnd = (n) => { s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0; return s % n; };
  const { instance } = await WebAssembly.instantiate(wasm, {});
  const ex = instance.exports;
  ex.sim_init(44100);
  const run = (ms) => { for (let i = 0; i < ms; i += 2) ex.sim_render(88); };
  run(300);
  let bad = "";
  const check = () => {
    const mode = ex.sim_ui(0), ov = ex.sim_ui(1), solo = ex.sim_ui(2), cat = ex.sim_ui(9);
    if (mode < 0 || mode > 2) bad = "mode " + mode;
    if (ov < 0 || ov > 5) bad = "overlay " + ov;
    if (solo < 0 || solo > 2) bad = "solo " + solo;
    if (cat < 1 || cat > 16) bad = "category " + cat;
    for (let p = 0; p < 4; p++) if (ex.sim_part(p, 5) > 8) bad = "voices " + ex.sim_part(p, 5);
    const l = new Float32Array(ex.memory.buffer, ex.sim_out_l(), 88);
    for (const v of l) if (!Number.isFinite(v) || Math.abs(v) > 4) bad = "sample " + v;
  };
  const btns = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
  try {
    for (let i = 0; i < steps && !bad; i++) {
      const r = rnd(100);
      if (r < 22) { const k = 5 + rnd(27); ex.sim_velocity(1 + rnd(127)); ex.sim_key(k, 1); run(2 + rnd(30)); ex.sim_key(k, 0); }
      else if (r < 40) { const p = rnd(16); ex.sim_pad(p, 1 + rnd(127)); run(2 + rnd(rnd(4) ? 40 : 900)); ex.sim_pad(p, 0); }
      else if (r < 62) { const b = btns[rnd(btns.length)]; ex.sim_button(b, 1); run(2 + rnd(rnd(5) ? 30 : 800)); ex.sim_button(b, 0); }
      else if (r < 78) { ex.sim_enc(rnd(7), rnd(9) - 4); }
      else if (r < 84) { ex.sim_fader(rnd(4), rnd(128)); }
      else if (r < 88) { ex.sim_button(0, 1); run(20); ex.sim_button(1, 1); run(20); ex.sim_button(0, 0); ex.sim_button(1, 0); }
      else if (r < 92) { ex.sim_midi(0x90, 36 + rnd(60), rnd(128)); }
      else if (r < 94) { ex.sim_midi(0xB0, [1, 7, 64, 123][rnd(4)], rnd(128)); }
      else if (r < 96) { ex.sim_wheel(rnd(2), 400 + rnd(250)); }
      else if (r < 97) { ex.sim_pedal(rnd(2) * 1000); }
      else if (r < 99) { ex.sim_load_patch(ex.sim_patch_id(rnd(ex.sim_patch_n()))); }
      run(2 + rnd(12));
      check();
    }
    if (!bad) { ex.sim_panic(); run(3000); check(); }
  } catch (e) { bad = "trap: " + e.message; }
  console.log((bad ? "  FAIL  " : "  ok    ") + "seed " + seed + " (" + steps + " random events)" + (bad ? "  " + bad : ""));
  if (bad) fail++;
}
process.exit(fail ? 1 : 0);
