// SPDX-License-Identifier: GPL-3.0-only
// The installer against a fake keyboard that speaks the update protocol (the keyboard pulls the image, the host answers):
// source firmware rules, identity guards, an interrupted write and its resume, and the restore path. Time runs 50x faster.
//   node tests/installer_test.mjs
import { createHash } from "node:crypto";
import { InstallService, InstallError, classify, chooseOperation, validatePackage, validateKit, LOADER, PENDING } from "../web/install/service.js";
import { pack7, unpack7 } from "../web/install/protocol.js";
import { crc16, productOf, logicalImage } from "../web/install/package.js";

const SCALE = 50, realST = globalThis.setTimeout, t0 = Date.now(), realNow = Date.now.bind(Date);
globalThis.setTimeout = (f, ms = 0, ...a) => realST(f, Math.min(ms / SCALE, 20), ...a);
Date.now = () => t0 + (realNow() - t0) * SCALE;

let fail = 0;
const check = (n, ok, i = "") => { console.log((ok ? "  ok    " : "  FAIL  ") + n + (i !== "" ? "  (" + i + ")" : "")); if (!ok) fail++; };
const rejects = async (p) => { try { await p; return null; } catch (e) { return e; } };

// ---- a small package with the real structure: encrypted header and entry, CRC-16, identity markers
function jlEnc(b) { const o = Uint8Array.from(b); let k = 0xFFFF; for (let i = 0; i < o.length; i++) { o[i] ^= k & 0xFF; k = ((k << 1) ^ (k & 0x8000 ? 0x1021 : 0)) & 0xFFFF; } return o; }
function makePackage(product, seed) {
  const payload = Uint8Array.from({ length: 6000 }, (_, i) => ((i * 7 + seed) % 120) + 1);   // never 0x7D (an unused-marker byte)
  const entry = new Uint8Array(0x50); const dv = new DataView(entry.buffer);
  dv.setUint16(0, 0, true); dv.setUint16(4, crc16(payload), true); dv.setUint32(8, 0x90, true); dv.setUint32(12, payload.length, true);
  const header = new Uint8Array(0x40); header[8] = 1;
  new DataView(header.buffer).setUint16(0, crc16(header.subarray(2)), true);
  const img = new Uint8Array(0x90 + payload.length);
  img.set(jlEnc(header), 0); img.set(jlEnc(entry), 0x40); img.set(payload, 0x90);
  const out = new Uint8Array(img.length + 20);
  for (let i = 0; i < 20; i++) {
    out.set(img.subarray(i * 0x2F, i * 0x2F + 0x2F), i * 0x30);
    out[i * 0x30 + 0x2F] = i < product.length ? (product.charCodeAt(i) + i + 1) & 0xFF : 0x7D;
  }
  out.set(img.subarray(20 * 0x2F), 20 * 0x30);
  return out;
}
const sha = (b) => createHash("sha256").update(b).digest("hex");
const OURS = makePackage("SMK-37 Pro_20001", 3), OLD = makePackage("SMK-37 Pro_19649", 11), STOCK = makePackage("SMK-37 Pro_016", 29);
const PIN = { name: "LiveGomezSynth test", product: "SMK-37 Pro_20001", size: OURS.length, sha256: sha(OURS) };

// ---- the fake keyboard
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7], UPG = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
class Keyboard {
  constructor(identity, { dropAfter = Infinity, loaderId = LOADER, loaderIs = null } = {}) {
    this.identity = identity; this.mode = identity.startsWith("ota-") ? "loader" : "app"; this.opts = { dropAfter, loaderId };
    this.served = 0; this.upgradeStarted = false; this.wrote = null; this.waiting = null; this.target = null;
    this.input = { name: "SMK-37 Pro", state: "connected", open: async () => {}, onmidimessage: null };
    this.output = { name: "SMK-37 Pro", state: "connected", open: async () => {}, send: (b) => this.receive(Array.from(b)) };
    this.access = { inputs: new Map([["i", this.input]]), outputs: new Map([["o", this.output]]) };
  }
  emit(bytes) { realST(() => this.input.onmidimessage && this.input.onmidimessage({ data: Uint8Array.from(bytes) }), 0); }
  idPacket() { const d = new Array(34).fill(0); d[1] = 0x59; d[2] = 0x11; [...this.identity].forEach((c, i) => { d[6 + i] = c.charCodeAt(0); }); return [0xF0, ...pack7(d), 0xF7]; }
  receive(b) {
    if (this.input.state === "disconnected") throw new Error("disconnected");
    const same = (x) => x.length === b.length && x.every((v, i) => v === b[i]);
    if (same(HS)) return this.emit(this.idPacket());
    if (same(UPG)) { this.upgradeStarted = true; return void this.session(); }
    const u = unpack7(b.slice(1, -1));
    if (u[1] === 0x59 && u[2] === 0x30 && this.waiting) { const w = this.waiting; this.waiting = null; w(u); }
  }
  ask(addr, len) {
    const u = [0, 0x59, 0x30, 0, 0, 0, 0, addr & 255, (addr >>> 8) & 255, (addr >>> 16) & 255, (addr >>> 24) & 255, len & 255, (len >> 8) & 255, 0, 0];
    let s = 0; for (let i = 6; i < 14; i++) s += u[i]; u[14] = ~s & 0xFF;
    return new Promise((ok) => { this.waiting = ok; this.emit([0xF0, ...pack7(u), 0xF7]); });
  }
  async serveRead(addr, len, image) {
    const r = await this.ask(addr, len), got = r.slice(14, 14 + len);
    return got.length === len && got.every((v, i) => v === image[addr + i]);
  }
  async session() {
    const image = this.target.image;
    if (this.mode === "app") {
      let ok = true;
      for (const [a, l] of [[0, 512], [512, 512]]) ok = (await this.serveRead(a, l, image)) && ok;
      await this.ask(0xE0000000, 8);
      this.checked = ok;
      await new Promise((r) => realST(r, 5));
      this.mode = "loader"; this.identity = this.opts.loaderId;
    } else {
      let ok = true, n = 0;
      for (let a = 0; a < image.length; a += 512) {
        if (n++ >= this.opts.dropAfter) { this.input.state = "disconnected"; this.output.state = "disconnected"; return; }
        ok = (await this.serveRead(a, Math.min(512, image.length - a), image)) && ok;
      }
      await this.ask(0xF0000000, 8);
      this.wrote = ok;
      await new Promise((r) => realST(r, 5));
      this.mode = "app"; this.identity = this.target.product;
    }
  }
}
const store = () => { const m = new Map(); return { getItem: (k) => (m.has(k) ? m.get(k) : null), setItem: (k, v) => m.set(k, String(v)), removeItem: (k) => m.delete(k), has: (k) => m.has(k) }; };
const prep = (kb, pkg) => { kb.target = { image: logicalImage(pkg), product: productOf(pkg) }; return kb; };

// ---- source rules
check("classify: SMKSYNTH 1.2.0", classify("SMK-37 Pro_19649") === "smksynth");
check("classify: ours", classify("SMK-37 Pro_20001") === "ours");
check("classify: stock V016", classify("SMK-37 Pro_016") === "stock");
check("classify: the update loader", classify(LOADER) === "loader");
check("classify: another firmware", classify("SMK-37_931") === "other" && classify("FM-1_904") === "other");
check("package: the pinned file passes", !!(await validatePackage(OURS, PIN)));
check("package: another file is refused", (await rejects(validatePackage(OLD, PIN))) instanceof InstallError);
const flipped = Uint8Array.from(OURS); flipped[400] ^= 1;
check("package: one flipped bit is refused", (await rejects(validatePackage(flipped, PIN))) instanceof InstallError);
check("kit: a firmware package is accepted", (await validateKit(STOCK)).product === "SMK-37 Pro_016");
check("kit: a random file is refused", (await rejects(validateKit(new Uint8Array(5000)))) instanceof InstallError);
check("operation: stock keyboard refused", (await rejects(Promise.resolve().then(() => chooseOperation("SMK-37 Pro_016", null, PIN))))?.code === "stock");
check("operation: unknown loader without a pending install refused", (await rejects(Promise.resolve().then(() => chooseOperation(LOADER, null, PIN))))?.code === "pending");
check("operation: loader with a matching pending install resumes", chooseOperation(LOADER, { sha: PIN.sha256, product: PIN.product }, PIN).mode === "resume");

// ---- install from SMKSYNTH
{
  const kb = prep(new Keyboard("SMK-37 Pro_19649"), OURS), st = store(), svc = new InstallService(kb.access, st);
  const id = await svc.identify();
  const steps = [];
  const product = await svc.install(PIN, OURS, { source: id.text }, (s) => steps.push(s));
  check("install: the keyboard reports the new identity", product === "SMK-37 Pro_20001" && kb.identity === "SMK-37 Pro_20001", kb.identity);
  check("install: the keyboard read the right bytes in both steps", kb.checked === true && kb.wrote === true);
  check("install: the pending marker is cleared", !st.has(PENDING));
  check("install: steps start, loader, write, reboot, done", ["start", "loader", "write", "reboot", "done"].every((s) => steps.includes(s)), steps.filter((s, i) => steps.indexOf(s) === i).join(","));
}
// ---- refusals send nothing
{
  const kb = prep(new Keyboard("SMK-37 Pro_016"), OURS), svc = new InstallService(kb.access, store());
  const e = await rejects(svc.install(PIN, OURS, { source: "SMK-37 Pro_016" }));
  check("stock keyboard: refused, nothing sent", e?.code === "stock" && !kb.upgradeStarted, e?.code);
}
{
  const kb = prep(new Keyboard("SMK-37 Pro_19649"), OURS), svc = new InstallService(kb.access, store());
  const e = await rejects(svc.install(PIN, OURS, { source: "SMK-37 Pro_19640" }));
  check("identity differs from what the page showed: refused, nothing sent", e?.code === "changed" && !kb.upgradeStarted, e?.code);
}
{
  const kb = prep(new Keyboard("SMK-37 Pro_19649"), OURS), svc = new InstallService(kb.access, store());
  const e = await rejects(svc.install(PIN, flipped, { source: "SMK-37 Pro_19649" }));
  check("damaged package: refused, nothing sent", e?.code === "package" && !kb.upgradeStarted, e?.code);
}
{
  const kb = prep(new Keyboard("SMK-37 Pro_19649", { loaderId: "ota-SMK-37 Pro_901" }), OURS), svc = new InstallService(kb.access, store());
  const e = await rejects(svc.install(PIN, OURS, { source: "SMK-37 Pro_19649" }));
  check("a loader of another identity is refused before the write", !!e && kb.wrote === null, e?.message);
}
// ---- interrupted write, then resume
{
  const kb = prep(new Keyboard("SMK-37 Pro_19649", { dropAfter: 3 }), OURS), st = store(), svc = new InstallService(kb.access, st);
  const e = await rejects(svc.install(PIN, OURS, { source: "SMK-37 Pro_19649" }));
  check("a cut write fails and keeps the pending marker", !!e && st.has(PENDING), e?.message?.slice(0, 50));
  const kb2 = prep(new Keyboard(LOADER), OURS), svc2 = new InstallService(kb2.access, st);
  const id = await svc2.identify();
  check("the keyboard in update mode is recognised", classify(id.text) === "loader", id.text);
  const product = await svc2.install(PIN, OURS, { source: id.text });
  check("resume writes the package and clears the marker", product === "SMK-37 Pro_20001" && kb2.wrote === true && !st.has(PENDING));
}
// ---- restore
{
  const kb = prep(new Keyboard("SMK-37 Pro_20001"), OLD), svc = new InstallService(kb.access, store());
  const kit = await validateKit(OLD);
  const product = await svc.restore(kit, { source: "SMK-37 Pro_20001" });
  check("restore: the keyboard returns to the saved firmware", product === "SMK-37 Pro_19649" && kb.identity === "SMK-37 Pro_19649" && kb.wrote === true, kb.identity);
}
{
  const kb = prep(new Keyboard("SMK-37 Pro_19649"), OLD), svc = new InstallService(kb.access, store());
  const e = await rejects(svc.restore(await validateKit(OLD), { source: "SMK-37 Pro_19649" }));
  check("restore: refused on a keyboard that does not run LiveGomezSynth", e?.code === "source" && !kb.upgradeStarted, e?.code);
}
// ---- no keyboard
{
  const svc = new InstallService({ inputs: new Map(), outputs: new Map() }, store());
  const e = await rejects(svc.identify());
  check("no keyboard: a clear error", e?.code === "notfound");
}
process.exit(fail ? 1 : 0);
