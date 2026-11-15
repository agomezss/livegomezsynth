// SPDX-License-Identifier: GPL-3.0-only
// LiveGomezSynth in an AudioWorklet: lgs.wasm is the firmware (firmware/src/lgs_app.c) on a simulated SMK-37
// (web/sim/sim.c). Each 128-frame render quantum runs the device's clock: the ISR's 256-frame blocks are served out of
// sim_render, the main loop runs once per millisecond of it. ~30 times a second the screen (when it changed), the LEDs
// and a few state probes go to the page.
// After SMKSYNTH / Makkuro37 / winampFM web simulators (GPL-3.0).
class LGS extends AudioWorkletProcessor {
  constructor(options) {
    super();
    const o = options.processorOptions || {};
    this.ex = null;
    this.fbHash = -1;
    this.lastPost = 0;
    try {
      const mod = new WebAssembly.Module(o.wasm);
      this.ex = new WebAssembly.Instance(mod, {}).exports;
      // the image must be in place before sim_init: the firmware's start-up reads it
      if (o.db && o.db.byteLength === this.ex.sim_db_size()) {
        new Uint8Array(this.ex.memory.buffer, this.ex.sim_stage(), o.db.byteLength).set(new Uint8Array(o.db));
        this.ex.sim_stage_set(o.db.byteLength);
      }
      this.ex.sim_splash_set(5000);   // the start-up splash, like the keyboard
      if (this.ex.sim_init(44100) !== 0) throw new Error("sim_init failed");
      if (o.master !== undefined) this.ex.sim_master(o.master);
      this.port.postMessage({ t: "ready" });
    } catch (err) {
      this.ex = null;
      this.port.postMessage({
        t: "error",
        message: String((err && err.message) || err),
      });
    }
    this.port.onmessage = (e) => this.onMessage(e.data);
  }
  onMessage(m) {
    const ex = this.ex;
    if (!ex) return;
    if (m.t === "key") ex.sim_key(m.k, m.down ? 1 : 0);
    else if (m.t === "btn") ex.sim_button(m.b, m.down ? 1 : 0);
    else if (m.t === "enc") ex.sim_enc(m.e, m.steps | 0);
    else if (m.t === "midi") ex.sim_midi(m.d[0] | 0, m.d[1] | 0, m.d[2] | 0);
    else if (m.t === "master") ex.sim_master(m.v | 0);
    else if (m.t === "pad") ex.sim_pad(m.p, m.v | 0);
    else if (m.t === "fader") ex.sim_fader(m.f, m.v | 0);
    else if (m.t === "wheel") ex.sim_wheel(m.w, m.v | 0);
    else if (m.t === "pedal") ex.sim_pedal(m.v | 0);
    else if (m.t === "vel") ex.sim_velocity(m.v | 0);
    else if (m.t === "panic") ex.sim_panic();
  }
  publish() {
    const ex = this.ex,
      tr = [];
    const msg = {
      t: "frame",
      leds: ex.sim_leds_buttons(),
      pads: [],
      prog: ex.sim_prog(),
      voices: ex.sim_voices(),
      peak: ex.sim_peak(),
      master: ex.sim_master_current(),
      ms: ex.sim_ms(),
    };
    for (let i = 0; i < 16; i++) msg.pads.push(ex.sim_pad_rgb(i));
    const h = ex.sim_fb_hash();
    if (h !== this.fbHash) {
      this.fbHash = h;
      msg.fb = new Uint16Array(
        ex.memory.buffer,
        ex.sim_fb(),
        240 * 240,
      ).slice();
      tr.push(msg.fb.buffer);
    }
    if (ex.sim_db_dirty()) {
      const db = new Uint8Array(ex.memory.buffer, ex.sim_db_export(), ex.sim_db_size()).slice();
      msg.db = db.buffer;
      tr.push(msg.db);
    }
    this.port.postMessage(msg, tr);
  }
  process(inputs, outputs) {
    const ex = this.ex;
    if (!ex) return true;
    const out = outputs[0],
      n = out[0].length;
    ex.sim_render(n);
    out[0].set(new Float32Array(ex.memory.buffer, ex.sim_out_l(), n));
    if (out[1])
      out[1].set(new Float32Array(ex.memory.buffer, ex.sim_out_r(), n));
    if (currentTime - this.lastPost >= 1 / 30) {
      this.lastPost = currentTime;
      this.publish();
    }
    return true;
  }
}
registerProcessor("lgs", LGS);
