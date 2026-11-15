// SPDX-License-Identifier: GPL-3.0-only
import {
  InstallService,
  InstallError,
  classify,
  chooseOperation,
  validateKit,
  sha256,
} from "./service.js";

const $ = (id) => document.getElementById(id);
const lines = [];
const log = (s) => {
  lines.push(new Date().toISOString() + " " + s);
  $("log").textContent = lines.join("\n");
};
const say = (id, text, kind = "") => {
  const e = $(id);
  e.textContent = text;
  e.className = "status " + kind;
};
const state = {
  service: null,
  id: null,
  op: null,
  pin: null,
  pkg: null,
  kit: null,
  kitRaw: null,
  busy: false,
};

// ---- the restore kit lives in the browser (IndexedDB), next to a downloadable copy
const idb = () =>
  new Promise((ok, no) => {
    const r = indexedDB.open("livegomezsynth", 1);
    r.onupgradeneeded = () => r.result.createObjectStore("kit");
    r.onsuccess = () => ok(r.result);
    r.onerror = () => no(r.error);
  });
const kitPut = async (v) => {
  const db = await idb();
  return new Promise((ok, no) => {
    const t = db.transaction("kit", "readwrite");
    t.objectStore("kit").put(v, "kit");
    t.oncomplete = ok;
    t.onerror = () => no(t.error);
  });
};
const kitGet = async () => {
  const db = await idb();
  return new Promise((ok, no) => {
    const r = db.transaction("kit").objectStore("kit").get("kit");
    r.onsuccess = () => ok(r.result || null);
    r.onerror = () => no(r.error);
  });
};
const download = (name, data, type) => {
  const u = URL.createObjectURL(new Blob([data], { type }));
  const a = document.createElement("a");
  a.href = u;
  a.download = name;
  a.click();
  setTimeout(() => URL.revokeObjectURL(u), 1000);
};

// the first install over SMKSYNTH is the only case that needs a way back kept here; an update of LiveGomezSynth and a
// write into the update loader (rescue) need nothing else
const hasMidi = !!navigator.requestMIDIAccess;
const needsBackup = () => !!state.op && state.op.where === "smksynth";

function refresh() {
  const known = !!state.id,
    ours = known && classify(state.id.text) === "ours",
    backup = needsBackup();
  $("s2").hidden = !backup;
  $("connect").disabled = state.busy || !hasMidi;
  $("kitFile").disabled = state.busy || !known;
  $("kitCopy").disabled = !state.kitRaw;
  $("bankBackup").disabled = $("stockKnown").disabled =
    state.busy || !state.kit;
  const backedUp =
    !backup ||
    (state.kit &&
      state.kit.product === state.id.text &&
      $("bankBackup").checked &&
      $("stockKnown").checked);
  const ready = known && state.op && state.pin && backedUp;
  $("install").disabled = state.busy || !ready;
  $("restore").disabled = state.busy || !(ours && state.kit);
}

// what the page found, in plain words: [status line, advice, advice kind]
function describe(where, rescue) {
  const v = state.pin ? state.pin.version : "";
  if (where === "ours")
    return [
      `LiveGomezSynth found. Install updates it to ${v}.`,
      "Your favourites and performances are kept (the release notes say if a version resets them). Nothing else to prepare: press Install.",
      "",
    ];
  if (where === "smksynth")
    return [
      `SMKSYNTH found: this is the first install of LiveGomezSynth.`,
      "Next: keep your SMKSYNTH file and make sure your banks are saved (the box below), then Install. After the first install, later updates need only one click.",
      "",
    ];
  if (where === "loader" || rescue)
    return [
      "The keyboard is in its update loader (rescue mode): Install writes this package into it.",
      "This is the normal way to get out of a keyboard that does not accept updates. Do not unplug until the page says Installed.",
      "",
    ];
  if (where === "recovery")
    return [
      "The keyboard is on its USB RESCUE screen: Install writes this package.",
      "Nothing else to prepare. Your favourites and performances are kept unless the release notes say otherwise. Do not unplug until the page says Installed.",
      "",
    ];
  if (where === "stock")
    return [
      "This keyboard runs the original M-VAVE firmware.",
      "LiveGomezSynth installs over SMKSYNTH, not over the original firmware (a first-install package for it does not exist yet). Install SMKSYNTH first, then come back to this page.",
      "bad",
    ];
  return [
    "A firmware this page does not know.",
    "Nothing was sent to the keyboard. LiveGomezSynth installs over SMKSYNTH or over itself. To start from another firmware, go back to the original with M-UPGRADE and install SMKSYNTH first.",
    "bad",
  ];
}

async function loadPin() {
  try {
    const m = await (
      await fetch("manifest.json", { cache: "no-store" })
    ).json();
    const raw = new Uint8Array(
      await (await fetch(m.url, { cache: "no-store" })).arrayBuffer(),
    );
    state.pin = m;
    state.pkg = raw;
    const dl = $("pkgDownload");
    dl.href = m.url;
    dl.download = m.url.split("/").pop();
    dl.textContent = `Download firmware ${m.version} (.fwsc)`;
    dl.hidden = false;
    say(
      "pin",
      `Package: ${m.name} (${m.product}), ${m.size} bytes, SHA-256 ${m.sha256.slice(0, 16)}...`,
    );
  } catch (e) {
    say("pin", "The package is not available here: " + e.message, "bad");
    log("manifest: " + e.message);
  }
  refresh();
}

$("connect").onclick = async () => {
  if (state.busy) return;
  state.busy = true;
  state.id = state.op = null;
  refresh();
  try {
    if (!navigator.requestMIDIAccess)
      throw new InstallError("browser", "use desktop Chrome or Edge");
    const access = await navigator.requestMIDIAccess({ sysex: true });
    state.service = new InstallService(access, localStorage, log);
    access.onstatechange = () => {
      if (!state.busy) {
        state.id = state.op = null;
        say("device", "USB changed: connect again.", "bad");
        refresh();
      }
    };
    state.id = await state.service.identify();
    const k = await kitGet().catch(() => null); // a kit saved in an earlier visit is used for restore
    if (k && !state.kit) {
      try {
        state.kit = await validateKit(k.raw);
        state.kitRaw = k.raw;
        say("kitState", `${k.product} from an earlier visit`, "good");
      } catch {
        /* damaged copy: choose the file again */
      }
    }
    say("device", `Keyboard: ${state.id.text}`, "good");
    log("identity " + state.id.text);
    state.op = null;
    $("help").hidden = true;
    const where = classify(state.id.text);
    const [plan, advice, kind] = describe(where);
    say("plan", plan, kind === "bad" ? "bad" : "good");
    $("advice").textContent = advice;
    $("advice").className = "advice " + kind;
    $("advice").hidden = false;
    try {
      state.op = state.pin
        ? chooseOperation(state.id.text, state.service.readPending(), state.pin)
        : null;
    } catch (e) {
      log(e.message);
    }
  } catch (e) {
    const blocked = e.name === "SecurityError" || e.name === "NotAllowedError";
    say(
      "device",
      blocked
        ? "The browser blocked MIDI access. Allow MIDI with system exclusive for this page and press the button again."
        : e.message,
      "bad",
    );
    log(e.message);
  } finally {
    state.busy = false;
    refresh();
  }
};

const saveRecord = () => {
  const rec = {
    tool: "LiveGomezSynth installer",
    time: new Date().toISOString(),
    keyboard: state.id.text,
    package: state.pin
      ? { product: state.pin.product, sha256: state.pin.sha256 }
      : null,
    agent: navigator.userAgent,
  };
  download(
    "livegomezsynth-record-" + state.id.text.replace(/\W+/g, "-") + ".json",
    JSON.stringify(rec, null, 2),
    "application/json",
  );
};

$("kitFile").onchange = async (ev) => {
  const f = ev.target.files[0];
  if (!f) return;
  try {
    const raw = new Uint8Array(await f.arrayBuffer()),
      kit = await validateKit(raw);
    if (kit.product !== state.id.text)
      throw new InstallError(
        "kit",
        `this package is ${kit.product}, the keyboard runs ${state.id.text}: pick the package of the firmware you run now`,
      );
    await kitPut({ raw, product: kit.product, sha: kit.sha, name: f.name });
    state.kit = kit;
    state.kitRaw = raw;
    say(
      "kitState",
      `${kit.product}, ${kit.size} bytes, SHA-256 ${kit.sha.slice(0, 16)}... saved in this browser`,
      "good",
    );
    log("restore kit " + kit.product + " " + kit.sha);
  } catch (e) {
    state.kit = state.kitRaw = null;
    say("kitState", e.message, "bad");
    log(e.message);
  }
  refresh();
};
$("kitCopy").onclick = () =>
  download(
    "restore-" + state.kit.product.replace(/\W+/g, "-") + ".fwsc",
    state.kitRaw,
    "application/octet-stream",
  );
$("bankBackup").onchange = $("stockKnown").onchange = refresh;

function ask(title, text, word) {
  return new Promise((ok) => {
    $("confirmTitle").textContent = title;
    $("confirmText").textContent = text;
    $("word").textContent = word;
    $("typed").value = "";
    $("go").disabled = true;
    $("typed").oninput = () => {
      $("go").disabled = $("typed").value.trim() !== word;
    };
    $("confirm").onclose = () => ok($("confirm").returnValue === "confirm");
    $("confirm").showModal();
  });
}

const STEPS = {
  start: "Starting the update loader...",
  verify: "The keyboard is checking the package...",
  loader: "Waiting for the update loader...",
  write: "Writing",
  reboot: "Restarting...",
  done: "Done",
};
function stepper(total) {
  return (name, n) => {
    const text =
      name === "write"
        ? `Writing: ${n} requests`
        : name === "verify"
          ? `Checking: ${n} requests`
          : STEPS[name] || name;
    say("step", text);
    log(text + (name === "done" ? " " + n : ""));
    $("progress").value =
      name === "write"
        ? Math.min(95, 10 + (n * 512 * 85) / total)
        : name === "done"
          ? 100
          : $("progress").value;
  };
}

async function run(title, text, word, job) {
  if (state.busy || !(await ask(title, text, word))) return;
  state.busy = true;
  $("help").hidden = true;
  refresh();
  try {
    if (!navigator.locks?.request)
      throw new InstallError("browser", "use an up-to-date Chrome or Edge");
    await navigator.locks.request(
      "livegomezsynth-firmware-update",
      { ifAvailable: true },
      async (lock) => {
        if (!lock)
          throw new InstallError("busy", "another tab is running an update");
        await job();
      },
    );
  } catch (e) {
    say("step", e.message, "bad");
    log("ERROR " + (e.code || "") + " " + e.message);
    showHelp(e);
  } finally {
    state.busy = false;
    refresh();
  }
}

// what to do next, for the failures that have a known remedy
function showHelp(e) {
  const h = $("help");
  let t = "";
  if (e.code === "stopped" && /after 0 requests/.test(e.message))
    t =
      "The keyboard did not accept the update command. Some older versions (LiveGomezSynth 0.1.5 and 0.1.6) cannot be updated from the page; do this once:\n1. Switch the keyboard off.\n2. Hold OCTAVE- and switch it on again. The screen stays dark: that is the update loader.\n3. Press Connect and identify here again, then Install.\nAfter that, updates work with one click.";
  else if (e.code === "noloader")
    t =
      "Unplug the USB cable, plug it again, press Connect and identify and then Install: the keyboard waits in update mode until the install is finished.";
  else if (e.code === "lost" || e.code === "noreturn")
    t =
      "Do not switch anything off yet. Wait 30 seconds, then press Connect and identify: if the keyboard shows up in update mode, Install resumes the write.";
  h.textContent = t;
  h.className = "advice bad";
  h.hidden = !t;
}

$("install").onclick = () =>
  run(
    "Install " + state.pin.name,
    `The keyboard (${state.id.text}) ${state.op.rescue ? "is in its update loader" : "will restart into the update loader"} and the firmware will be written (about ${Math.round(state.pin.size / 1024)} KB). Do not unplug or close this page. ` +
      (needsBackup()
        ? "The first start clears the SMKSYNTH programs and favourites."
        : "Your favourites and performances are kept unless the release notes say otherwise."),
    "INSTALL",
    async () => {
      if (needsBackup()) saveRecord(); // the identity record: what the keyboard reported and when, kept by the browser download
      const product = await state.service.install(
        state.pin,
        state.pkg,
        { source: state.id.text },
        stepper(state.pin.size),
      );
      say("step", `Installed: the keyboard reports ${product}.`, "good");
      state.id = { text: product, model: "SMK-37 Pro" };
      state.op = null;
      refresh();
    },
  );

$("restore").onclick = () =>
  run(
    "Restore " + state.kit.product,
    `Writes back ${state.kit.product} from the package you saved. Experimental: not tested on hardware. The official way back is M-UPGRADE.`,
    "RESTORE",
    async () => {
      const product = await state.service.restore(
        state.kit,
        { source: state.id.text },
        stepper(state.kit.size),
      );
      say("restoreState", `Restored: the keyboard reports ${product}.`, "good");
    },
  );

$("downloadLog").onclick = () =>
  download("livegomezsynth-install-log.txt", lines.join("\n"), "text/plain");
window.addEventListener("beforeunload", (e) => {
  if (state.busy) {
    e.preventDefault();
    e.returnValue = "";
  }
});
if (!hasMidi) {
  $("browser").textContent =
    "This page needs desktop Chrome or Edge (Web MIDI with system exclusive). Open it there.";
  $("browser").hidden = false;
}
loadPin();
refresh();
