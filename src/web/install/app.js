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
  recorded: false,
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

function refresh() {
  const known = !!state.id,
    ours = known && classify(state.id.text) === "ours";
  $("connect").disabled = state.busy;
  $("record").disabled = state.busy || !known;
  $("kitFile").disabled = state.busy || !known;
  $("kitCopy").disabled = !state.kitRaw;
  $("bankBackup").disabled = $("stockKnown").disabled =
    state.busy || !state.kit;
  const backedUp =
    state.op && state.op.where !== "smksynth"
      ? true
      : state.kit &&
        state.kit.product === state.id.text &&
        $("bankBackup").checked &&
        $("stockKnown").checked; // a keyboard on LiveGomezSynth already has its kit
  const ready = known && state.op && state.pin && state.recorded && backedUp;
  $("install").disabled = state.busy || !ready;
  $("restore").disabled = state.busy || !(ours && state.kit);
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
    try {
      state.op = state.pin
        ? chooseOperation(state.id.text, state.service.readPending(), state.pin)
        : null;
      say(
        "plan",
        state.op
          ? state.op.mode === "resume"
            ? "An earlier transfer was interrupted: Install resumes it."
            : `Ready to update from ${state.op.where === "ours" ? "LiveGomezSynth" : "SMKSYNTH"}.`
          : "",
        "good",
      );
    } catch (e) {
      say("plan", e.message, "bad");
      log(e.message);
    }
  } catch (e) {
    say("device", e.message, "bad");
    log(e.message);
  } finally {
    state.busy = false;
    refresh();
  }
};

$("record").onclick = () => {
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
  state.recorded = true;
  say("recordState", "saved", "good");
  refresh();
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
  } finally {
    state.busy = false;
    refresh();
  }
}

$("install").onclick = () =>
  run(
    "Install " + state.pin.name,
    `The keyboard (${state.id.text}) will restart into the update loader and the firmware will be written (about ${Math.round(state.pin.size / 1024)} KB). Do not unplug or close this page. The first start re-initializes the user-data bank.`,
    "INSTALL",
    async () => {
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
loadPin();
refresh();
