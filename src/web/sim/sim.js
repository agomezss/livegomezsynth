// SPDX-License-Identifier: GPL-3.0-only
// LiveGomezSynth simulator, the page: builds the SMK-37 panel (8 knobs, 4 faders, 16 RGB pads, 18 buttons, 37 keys),
// starts the AudioWorklet (worklet.js runs lgs.wasm, the firmware), draws its 240 x 240 screen, pad and button LEDs, and
// feeds it the mouse, touch, the computer keyboard and Web MIDI.
(function () {
  "use strict";
  const $ = (s, r = document) => r.querySelector(s);
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));

  // the firmware's numbers (firmware/src/platform.c: BTN_*; lgs_input.h: LGS_K*)
  const BTN = [
    ["KNOB BANK", 0],
    ["FADER BANK", 1],
    ["<", 2],
    [">", 3],
    ["OCT-", 4],
    ["OCT+", 5],
    ["ARP", 6],
    ["NOTE REPEAT", 7],
    ["SCALE", 8],
    ["CHORD", 9],
    ["GLOBE", 10],
    ["BT", 11],
    ["PATCH", 12],
    ["PARA", 13],
    ["FX", 14],
    ["SEQ", 15],
    ["SEQ PLAY", 16],
    ["SEQ REC", 17],
  ];
  const KNOB_LABELS = ["K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8 MASTER"];
  const FADER_LABELS = ["CHORUS", "DELAY", "DIST", "REVERB"];
  const S = {
    node: null,
    ctx: null,
    on: false,
    starting: false,
    master: 724,
    midi: null,
    midiSel: "",
  };

  // user data (favorites, performances): the firmware's own image, kept in localStorage
  const DB_KEY = "lgs-user-db-v1";
  const loadDb = () => {
    try {
      const t = localStorage.getItem(DB_KEY);
      if (!t) return null;
      const b = atob(t),
        a = new Uint8Array(b.length);
      for (let i = 0; i < b.length; i++) a[i] = b.charCodeAt(i);
      return a.buffer;
    } catch (e) {
      return null;
    }
  };
  const saveDb = (buf) => {
    try {
      let s = "";
      new Uint8Array(buf).forEach((x) => (s += String.fromCharCode(x)));
      localStorage.setItem(DB_KEY, btoa(s));
    } catch (e) {}
  };

  const send = (m) => {
    if (S.node) S.node.port.postMessage(m);
  };

  // ---- the screen
  const cv = $("#screen"),
    cx = cv.getContext("2d"),
    img = cx.createImageData(240, 240);
  const bigCx = $("#screenBig").getContext("2d");
  bigCx.imageSmoothingEnabled = false;
  const R5 = new Uint8Array(32).map((_, i) => ((i * 255) / 31) | 0),
    G6 = new Uint8Array(64).map((_, i) => ((i * 255) / 63) | 0);
  const draw = (fb) => {
    const d = img.data;
    for (let k = 0, j = 0; k < 57600; k++, j += 4) {
      const v = fb[k];
      d[j] = R5[v >> 11];
      d[j + 1] = G6[(v >> 5) & 63];
      d[j + 2] = R5[v & 31];
      d[j + 3] = 255;
    }
    cx.putImageData(img, 0, 0);
    bigCx.drawImage(cv, 0, 0, 480, 480);
  };
  cx.fillStyle = "#000";
  cx.fillRect(0, 0, 240, 240);

  // ---- power on
  const power = $("#power");
  const say = (a, b) => {
    power.querySelector(".p1").textContent = a;
    power.querySelector(".p2").textContent = b;
  };
  async function start() {
    if (S.on || S.starting) return;
    S.starting = true;
    say("Starting…", "loading the firmware");
    try {
      if (!window.AudioWorkletNode)
        throw new Error("this browser has no AudioWorklet");
      const Ctx = window.AudioContext || window.webkitAudioContext;
      try {
        S.ctx = new Ctx({ sampleRate: 44100, latencyHint: "interactive" });
      } catch (e) {
        S.ctx = new Ctx({ latencyHint: "interactive" });
      }
      if (S.ctx.sampleRate !== 44100)
        throw new Error(
          "the audio runs at " +
            S.ctx.sampleRate +
            " Hz, the firmware wants 44100",
        );
      const resumed = S.ctx.resume();
      const [wasm] = await Promise.all([
        fetch("lgs.wasm").then((r) => {
          if (!r.ok) throw new Error("lgs.wasm: " + r.status);
          return r.arrayBuffer();
        }),
        S.ctx.audioWorklet.addModule("worklet.js"),
      ]);
      S.node = new AudioWorkletNode(S.ctx, "lgs", {
        numberOfInputs: 0,
        outputChannelCount: [2],
        processorOptions: { wasm, master: S.master, db: loadDb() },
      });
      S.node.port.onmessage = (msg) => {
        const m = msg.data;
        if (m.t === "frame") {
          if (m.db) saveDb(m.db);
          frame(m);
        }
        else if (m.t === "ready") {
          S.on = true;
          power.hidden = true;
        } else if (m.t === "error") {
          say("Error", m.message);
          S.starting = false;
        }
      };
      S.node.connect(S.ctx.destination);
      await resumed;
    } catch (e) {
      S.starting = false;
      say("Could not start", String((e && e.message) || e));
    }
  }
  power.addEventListener("click", start);
  if (/[?&]autostart\b/.test(location.search)) start();
  const wake = () => {
    if (S.ctx && S.ctx.state === "suspended") S.ctx.resume();
  };
  ["pointerup", "touchend", "keydown", "click"].forEach((t) =>
    document.addEventListener(t, wake, true),
  );

  // ---- the panel
  const padEls = [],
    btnEls = {},
    keyEls = [];
  function frame(m) {
    if (m.fb) draw(m.fb);
    for (let i = 0; i < 16; i++) {
      const v = m.pads[i],
        r = (v >> 8) & 255,
        g = (v >> 16) & 255,
        b = v & 255;
      padEls[i].style.background = v ? `rgb(${r},${g},${b})` : "";
      padEls[i].style.boxShadow = v ? `0 0 ${6 + Math.max(r, g, b) / 20}px rgb(${r},${g},${b})` : "";
    }
    for (const i in btnEls)
      btnEls[i].classList.toggle("lit", !!((m.leds >> i) & 1));
    $("#state").textContent =
      `firmware time ${(m.ms / 1000).toFixed(1)} s\npatch ${m.prog}\nvoices ${m.voices}\npeak ${m.peak}\nmaster ${m.master}/1023`;
  }

  // knobs: endless encoders in detents (drag, wheel); K8 is the MASTER pot 0..1023 (24 per detent)
  const knobsEl = $("#knobs"),
    knobAng = [];
  KNOB_LABELS.forEach((label, e) => {
    const el = document.createElement("div");
    el.className = "knob" + (e === 7 ? " master" : "");
    el.innerHTML = '<div class="cap"></div><div class="lab"></div>';
    el.querySelector(".lab").textContent = label;
    const cap = el.querySelector(".cap");
    knobAng[e] = e === 7 ? (S.master / 1023) * 270 - 135 : 0;
    const show = () => {
      cap.style.transform = `rotate(${knobAng[e]}deg)`;
    };
    show();
    const turn = (steps) => {
      if (!steps) return;
      if (e === 7) {
        S.master = clamp(S.master + steps * 24, 0, 1023);
        knobAng[e] = (S.master / 1023) * 270 - 135;
        send({ t: "master", v: S.master });
      } else {
        knobAng[e] += steps * 15;
        send({ t: "enc", e, steps });
      }
      show();
    };
    let drag = null;
    el.addEventListener("pointerdown", (ev) => {
      ev.preventDefault();
      if (!S.on) start();
      el.setPointerCapture(ev.pointerId);
      drag = { y: ev.clientY, acc: 0 };
    });
    el.addEventListener("pointermove", (ev) => {
      if (!drag) return;
      drag.acc += drag.y - ev.clientY;
      drag.y = ev.clientY;
      const st = (drag.acc / 7) | 0;
      if (st) {
        drag.acc -= st * 7;
        turn(st);
      }
    });
    const end = () => {
      drag = null;
    };
    el.addEventListener("pointerup", end);
    el.addEventListener("pointercancel", end);
    let wacc = 0;
    el.addEventListener(
      "wheel",
      (ev) => {
        ev.preventDefault();
        wacc += ev.deltaMode === 0 ? ev.deltaY / 40 : ev.deltaY;
        const st = Math.trunc(wacc);
        if (st) {
          wacc -= st;
          turn(-st);
        }
      },
      { passive: false },
    );
    el.turn = turn;
    knobsEl.append(el);
    knobsEl.children[e].turn = turn;
  });
  const knobTurn = (e, st) => knobsEl.children[e].turn(st);

  // faders (0..127, sent to the firmware's soft-takeover logic when it uses them)
  const fadersEl = $("#faders");
  FADER_LABELS.forEach((label, f) => {
    const el = document.createElement("div");
    el.className = "fader";
    el.innerHTML =
      '<input type="range" min="0" max="127" value="0"><span></span>';
    el.title = label;
    el.querySelector("span").textContent = label;
    el.querySelector("input").addEventListener("input", (ev) => {
      if (!S.on) start();
      send({ t: "fader", f, v: +ev.target.value });
    });
    fadersEl.append(el);
  });

  // pads
  const padsEl = $("#pads");
  for (let p = 0; p < 16; p++) {
    const el = document.createElement("div");
    el.className = "pad";
    el.title = "pad " + (p + 1);
    el.addEventListener("pointerdown", (ev) => {
      ev.preventDefault();
      if (!S.on) start();
      el.setPointerCapture(ev.pointerId);
      el.classList.add("down");
      send({ t: "pad", p, v: 100 });
    });
    const up = () => {
      if (el.classList.contains("down")) {
        el.classList.remove("down");
        send({ t: "pad", p, v: 0 });
      }
    };
    el.addEventListener("pointerup", up);
    el.addEventListener("pointercancel", up);
    padsEl.append(el);
    padEls[p] = el;
  }

  // buttons
  const btnsEl = $("#btns"),
    btnHeld = new Set();
  // where the panel has them (docs/photo): the pair of columns under the screen, the 2 x 6 block right of the pads
  const LEFT = [0, 1, 2, 3, 4, 5];
  const SHORT = { "NOTE REPEAT": "REPEAT", "SEQ PLAY": "PLAY", "SEQ REC": "REC", "KNOB BANK": "KNOB BANK", "FADER BANK": "FADER BANK" };
  const btnDown = (b) => {
    if (btnHeld.has(b)) return;
    btnHeld.add(b);
    btnEls[b].classList.add("down");
    send({ t: "btn", b, down: 1 });
  };
  const btnUp = (b) => {
    if (!btnHeld.has(b)) return;
    btnHeld.delete(b);
    btnEls[b].classList.remove("down");
    send({ t: "btn", b, down: 0 });
  };
  BTN.forEach(([name, b]) => {
    const el = document.createElement("button");
    el.className = "btn";
    el.textContent = SHORT[name] || name;
    el.title = name;
    el.addEventListener("pointerdown", (ev) => {
      ev.preventDefault();
      if (!S.on) start();
      el.setPointerCapture(ev.pointerId);
      btnDown(b);
    });
    el.addEventListener("pointerup", () => btnUp(b));
    el.addEventListener("pointercancel", () => btnUp(b));
    el.addEventListener("contextmenu", (ev) => ev.preventDefault());
    (LEFT.includes(b) ? $("#btnsL") : $("#btnsR")).append(el);
    btnEls[b] = el;
  });
  const chord = document.createElement("button"); // KNOB BANK + FADER BANK together (a mouse has one pointer)
  chord.className = "btn chord";
  chord.textContent = "CHORD: KNOB+FADER BANK";
  chord.addEventListener("pointerdown", (ev) => {
    ev.preventDefault();
    if (!S.on) start();
    chord.setPointerCapture(ev.pointerId);
    btnDown(0);
    btnDown(1);
  });
  const chordUp = () => {
    btnUp(0);
    btnUp(1);
  };
  chord.addEventListener("pointerup", chordUp);
  chord.addEventListener("pointercancel", chordUp);
  btnsEl.append(chord);

  // keys: keybed 0..36 = C3..C6; the firmware's 27 note keys are keybed 5..31 (F3..G5), the others are the outer keys
  const keysEl = $("#keys"),
    NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const isBlack = (k) => [1, 3, 6, 8, 10].includes(k % 12);
  const whites = [];
  for (let k = 0; k < 37; k++) if (!isBlack(k)) whites.push(k);
  const WW = 100 / whites.length;
  let wi = 0;
  for (let k = 0; k < 37; k++) {
    const el = document.createElement("div"),
      black = isBlack(k);
    el.className =
      "key " + (black ? "b" : "w") + (k < 5 || k > 31 ? " outer" : "");
    el.dataset.k = k;
    el.innerHTML = '<span class="kl"></span>';
    if (k % 12 === 0) el.querySelector(".kl").textContent = "C" + (3 + k / 12);
    if (black) {
      el.style.left = `calc(${wi * WW}% - ${WW * 0.31}%)`;
      el.style.width = `${WW * 0.62}%`;
    } else {
      el.style.left = `${wi * WW}%`;
      el.style.width = `${WW}%`;
      wi++;
    }
    keysEl.append(el);
    keyEls[k] = el;
  }
  const keyHeld = new Map();
  const keyDown = (k) => {
    const n = keyHeld.get(k) || 0;
    keyHeld.set(k, n + 1);
    if (!n) {
      keyEls[k].classList.add("down");
      send({ t: "key", k, down: 1 });
    }
  };
  const keyUp = (k) => {
    const n = keyHeld.get(k) || 0;
    if (!n) return;
    if (n > 1) {
      keyHeld.set(k, n - 1);
      return;
    }
    keyHeld.delete(k);
    keyEls[k].classList.remove("down");
    send({ t: "key", k, down: 0 });
  };
  const ptrKey = new Map();
  const keyAt = (x, y) => {
    const el = document.elementFromPoint(x, y);
    const k = el && el.closest(".key");
    return k && keysEl.contains(k) ? +k.dataset.k : -1;
  };
  keysEl.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    if (!S.on) start();
    keysEl.setPointerCapture(e.pointerId);
    const k = keyAt(e.clientX, e.clientY);
    ptrKey.set(e.pointerId, k);
    if (k >= 0) keyDown(k);
  });
  keysEl.addEventListener("pointermove", (e) => {
    if (!ptrKey.has(e.pointerId)) return;
    const k = keyAt(e.clientX, e.clientY),
      was = ptrKey.get(e.pointerId);
    if (k === was) return;
    if (was >= 0) keyUp(was);
    if (k >= 0) keyDown(k);
    ptrKey.set(e.pointerId, k);
  });
  const ptrEnd = (e) => {
    if (!ptrKey.has(e.pointerId)) return;
    const k = ptrKey.get(e.pointerId);
    ptrKey.delete(e.pointerId);
    if (k >= 0) keyUp(k);
  };
  keysEl.addEventListener("pointerup", ptrEnd);
  keysEl.addEventListener("pointercancel", ptrEnd);

  $("#vel").addEventListener("input", (e) =>
    send({ t: "vel", v: +e.target.value }),
  );

  // ---- the computer keyboard (physical positions); firmware key n = keybed n + 5
  const CODEKEY = {
    KeyZ: 7,
    KeyS: 8,
    KeyX: 9,
    KeyD: 10,
    KeyC: 11,
    KeyV: 12,
    KeyG: 13,
    KeyB: 14,
    KeyH: 15,
    KeyN: 16,
    KeyJ: 17,
    KeyM: 18,
    Comma: 19,
    KeyL: 20,
    Period: 21,
    Semicolon: 22,
    Slash: 23,
    KeyQ: 0,
    Digit2: 1,
    KeyW: 2,
    Digit3: 3,
    KeyE: 4,
    Digit4: 5,
    KeyR: 6,
    KeyI: 24,
    Digit9: 25,
    KeyO: 26,
  };
  const CODEBTN = {
    Digit1: 0,
    Digit5: 1,
    ArrowLeft: 2,
    ArrowRight: 3,
    KeyA: 4,
    KeyF: 5,
    Digit7: 12,
    Minus: 13,
    Equal: 10,
    BracketLeft: 14,
    BracketRight: 11,
  };
  // sustain pedal (Space or the button), pitch and mod wheels (sliders; the pitch wheel springs back)
  const pedal = (down) => send({ t: "pedal", v: down ? 1000 : 0 });
  const pedBtn = $("#ped");
  pedBtn.addEventListener("pointerdown", () => pedal(true));
  ["pointerup", "pointerleave", "pointercancel"].forEach((t) =>
    pedBtn.addEventListener(t, () => pedal(false)),
  );
  const pw = $("#pw"),
    mw = $("#mw");
  pw.addEventListener("input", () => send({ t: "wheel", w: 0, v: 512 + Math.round(pw.value * 0.8) }));
  pw.addEventListener("change", () => {
    pw.value = 0;
    send({ t: "wheel", w: 0, v: 512 });
  });
  mw.addEventListener("input", () => send({ t: "wheel", w: 1, v: 620 - Math.round((mw.value * 135) / 127) }));
  const typing = (e) =>
    /input|textarea|select/i.test((e.target && e.target.tagName) || "");
  const codeHeld = new Set();
  addEventListener("keydown", (e) => {
    if (typing(e) || e.metaKey || e.ctrlKey || e.altKey) return;
    const k = CODEKEY[e.code],
      b = CODEBTN[e.code];
    if (e.code === "ArrowUp" || e.code === "ArrowDown") {
      e.preventDefault();
      if (!S.on) start();
      knobTurn(6, e.code === "ArrowUp" ? 1 : -1);
      return;
    }
    if (e.code === "Space") {
      e.preventDefault();
      if (!e.repeat) pedal(true);
      return;
    }
    if (k === undefined && b === undefined) return;
    e.preventDefault();
    if (e.repeat || codeHeld.has(e.code)) return;
    if (!S.on) start();
    codeHeld.add(e.code);
    if (k !== undefined) keyDown(k + 5);
    else btnDown(b);
  });
  addEventListener("keyup", (e) => {
    if (e.code === "Space") pedal(false);
    if (!codeHeld.has(e.code)) return;
    codeHeld.delete(e.code);
    const k = CODEKEY[e.code],
      b = CODEBTN[e.code];
    if (k !== undefined) keyUp(k + 5);
    else if (b !== undefined) btnUp(b);
  });
  addEventListener("blur", () => {
    for (const c of codeHeld) {
      const k = CODEKEY[c],
        b = CODEBTN[c];
      if (k !== undefined) keyUp(k + 5);
      else if (b !== undefined) btnUp(b);
    }
    codeHeld.clear();
    send({ t: "panic" });
  });

  // ---- Web MIDI
  $("#resetDb").addEventListener("click", () => {
    try {
      localStorage.removeItem(DB_KEY);
    } catch (e) {}
    location.reload();
  });
  const midiBtn = $("#midiBtn"),
    midiSel = $("#midiSel"),
    midiMsg = $("#midiMsg");
  const onMidi = (port) => (m) => {
    if (S.midiSel && port.id !== S.midiSel) return;
    const d = m.data,
      st = d[0];
    if (!S.on || st < 0x80 || st === 0xf0 || st === 0xf7 || st === 0xfe) return;
    send({ t: "midi", d: [st, d[1] || 0, d[2] || 0] });
  };
  const listMidi = () => {
    const acc = S.midi,
      keep = S.midiSel;
    midiSel.innerHTML = '<option value="">All MIDI inputs</option>';
    let n = 0;
    acc.inputs.forEach((p) => {
      n++;
      p.onmidimessage = onMidi(p);
      const o = document.createElement("option");
      o.value = p.id;
      o.textContent = p.name || p.id;
      midiSel.append(o);
    });
    midiSel.value = [...midiSel.options].some((o) => o.value === keep)
      ? keep
      : "";
    S.midiSel = midiSel.value;
    midiSel.disabled = false;
    midiMsg.textContent = n
      ? `${n} MIDI input${n > 1 ? "s" : ""} found.`
      : "No MIDI input found.";
  };
  midiBtn.addEventListener("click", async () => {
    if (!navigator.requestMIDIAccess) {
      midiMsg.textContent = "This browser has no Web MIDI.";
      return;
    }
    try {
      S.midi = await navigator.requestMIDIAccess({ sysex: false });
      S.midi.onstatechange = listMidi;
      listMidi();
      midiBtn.textContent = "MIDI connected";
      if (!S.on) start();
    } catch (e) {
      midiMsg.textContent = "MIDI was not allowed: " + ((e && e.message) || e);
    }
  });
  midiSel.addEventListener("change", () => {
    S.midiSel = midiSel.value;
  });
})();
