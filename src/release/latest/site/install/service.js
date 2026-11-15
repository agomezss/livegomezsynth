// SPDX-License-Identifier: GPL-3.0-only
// LiveGomezSynth installer logic: which firmware the keyboard may be updated from, package checks, install and restore.
// protocol.js and package.js are the reference installer's (SMKSYNTH/web/install, verbatim).
import { Updater } from "./protocol.js";
import { productOf, logicalImage, filesOk } from "./package.js";

export const MODEL = "SMK-37 Pro";
export const LOADER = "ota-SMK-37 Pro_900"; // the update loader this keyboard family runs (D-01, loader/ verbatim)
export const PENDING = "livegomezsynth.install.pending.v1";
// SMKSYNTH versions whose update loader the reference installer accepts for an update package
const SMKSYNTH_IDS = new Set([
  "19615",
  "19616",
  "19617",
  "19618",
  "19619",
  "19620",
  "19621",
  "19622",
  "19623",
  "19624",
  "19625",
  "19626",
  "19627",
  "19628",
  "19630",
  "19631",
  "19632",
  "19633",
  "19634",
  "19635",
  "19636",
  "19637",
  "19638",
  "19639",
  "19640",
  "19641",
  "19642",
  "19643",
  "19644",
  "19645",
  "19646",
  "19647",
  "19648",
  "19649",
]);

export class InstallError extends Error {
  constructor(code, msg) {
    super(msg);
    this.code = code;
  }
}
const hex = (a) =>
  Array.from(new Uint8Array(a), (v) => v.toString(16).padStart(2, "0")).join(
    "",
  );
export const sha256 = async (bytes) =>
  hex(await crypto.subtle.digest("SHA-256", bytes));

// where the keyboard stands: "ours" (a LiveGomezSynth, can be updated), "smksynth" (can be updated), "stock" (needs a first-install
// package, which this release does not have), "loader" (an update was interrupted), "other"
export function classify(idText) {
  let m = /^SMK-37 Pro_(\d{3,5})$/.exec(idText || "");
  if (m) {
    if (/^2\d{4}$/.test(m[1])) return "ours";
    if (SMKSYNTH_IDS.has(m[1])) return "smksynth";
    if (m[1] === "016") return "stock";
    return "other";
  }
  return idText === LOADER ? "loader" : "other";
}

// a package we may write: the pinned file from manifest.json
export async function validatePackage(raw, pin) {
  if (!pin || raw.length !== pin.size)
    throw new InstallError("package", "wrong package or size");
  if ((await sha256(raw)) !== pin.sha256)
    throw new InstallError(
      "package",
      "the package does not match the pinned SHA-256",
    );
  const product = productOf(raw),
    image = logicalImage(raw);
  if (product !== pin.product || !filesOk(image))
    throw new InstallError(
      "package",
      "package identity or integrity check failed",
    );
  return { product, image, sha: pin.sha256 };
}

// a restore kit: the firmware package the keyboard runs now (official stock or SMKSYNTH), chosen by the user
export async function validateKit(raw) {
  let product;
  try {
    product = productOf(raw);
  } catch (e) {
    throw new InstallError("kit", "not a firmware package");
  }
  const image = logicalImage(raw);
  if (!/^SMK-37 Pro_\d{3,5}$/.test(product))
    throw new InstallError(
      "kit",
      `the package is for ${product || "an unknown device"}, not an ${MODEL}`,
    );
  if (!filesOk(image))
    throw new InstallError(
      "kit",
      "the package failed its integrity check (damaged download?)",
    );
  return { product, image, sha: await sha256(raw), size: raw.length };
}

// what an install would do on this keyboard
export function chooseOperation(idText, pending, pin) {
  const where = classify(idText);
  if (where === "loader") {
    if (
      pending &&
      pending.sha === pin.sha256 &&
      pending.product === pin.product
    )
      return { mode: "resume", where, source: idText };
    // the rescue loader (OCTAVE- at power-on) or a transfer another browser started: write this package into it; the
    // updater checks that the loader's model is the package's model and the identity the keyboard reports afterwards
    return { mode: "resume", where, source: idText, rescue: true };
  }
  if (where === "stock")
    throw new InstallError(
      "stock",
      "the keyboard runs the M-VAVE firmware (" +
        idText +
        "). This release is an update package: it needs SMKSYNTH or LiveGomezSynth on the keyboard first",
    );
  if (where === "other")
    throw new InstallError(
      "source",
      "unsupported firmware on the keyboard: " + (idText || "none"),
    );
  return { mode: "install", where, source: idText };
}

export class InstallService {
  constructor(access, storage, log = () => {}, UpdaterType = Updater) {
    Object.assign(this, { access, storage, log, UpdaterType });
  }

  async identify() {
    const up = new this.UpdaterType(this.access, this.log);
    const d = await up.find();
    if (!d)
      throw new InstallError(
        "notfound",
        "keyboard not found: check the USB cable and close other MIDI applications",
      );
    d.link.close();
    return d.id;
  }

  readPending() {
    try {
      return JSON.parse(this.storage.getItem(PENDING));
    } catch {
      return null;
    }
  }

  // selection: what the page showed the user (the identity it saw); the keyboard must still be that one
  async install(pin, raw, selection, step = () => {}) {
    const pkg = await validatePackage(raw, pin);
    let current;
    const guard = (phase, id) => {
      if (phase === "start" && id.text !== current)
        throw new InstallError(
          "changed",
          "the keyboard identity changed before writing",
        );
      if (phase !== "start" && id.text !== LOADER)
        throw new InstallError(
          "loader",
          "an unexpected update loader was refused: " + id.text,
        );
    };
    const up = new this.UpdaterType(this.access, this.log, { guard });
    const d = await up.find();
    if (!d) throw new InstallError("notfound", "keyboard not found");
    current = d.id.text;
    d.link.close();
    if (current !== selection.source)
      throw new InstallError(
        "changed",
        "the keyboard changed; connect and check again",
      );
    const op = chooseOperation(current, this.readPending(), pin);
    this.storage.setItem(
      PENDING,
      JSON.stringify({
        sha: pin.sha256,
        product: pin.product,
        source: current,
      }),
    );
    if (op.mode === "resume") {
      if (!(await up.resume(pkg.image, step, { product: pkg.product })))
        throw new InstallError(
          "incomplete",
          "the resume did not finish: keep the log and try again",
        );
    } else await up.install(pkg.image, pkg.product, step);
    this.storage.removeItem(PENDING);
    return pkg.product;
  }

  // return to the firmware the kit holds (unverified on hardware: M-UPGRADE stays the official way back to stock)
  async restore(kit, selection, step = () => {}) {
    let current;
    const guard = (phase, id) => {
      if (phase === "start" && id.text !== current)
        throw new InstallError(
          "changed",
          "the keyboard identity changed before writing",
        );
      if (phase !== "start" && id.text !== LOADER)
        throw new InstallError(
          "loader",
          "an unexpected update loader was refused: " + id.text,
        );
    };
    const up = new this.UpdaterType(this.access, this.log, { guard });
    const d = await up.find();
    if (!d) throw new InstallError("notfound", "keyboard not found");
    current = d.id.text;
    d.link.close();
    if (current !== selection.source)
      throw new InstallError(
        "changed",
        "the keyboard changed; connect and check again",
      );
    if (classify(current) !== "ours")
      throw new InstallError(
        "source",
        "restore is for a keyboard that runs LiveGomezSynth",
      );
    await up.install(kit.image, kit.product, step);
    return kit.product;
  }
}
