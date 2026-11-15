#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Release assembly (T4.2): from a built package to a folder that can be published as it is.

  python3 tools/package_release.py [--version <src/VERSION>] [--bump] [--id 20000] [--build]

--build runs tools/lgs_build.py first (needs the JieLi toolchain, see README). Without it the package in build/ is used.
Writes release/<version>/ and a copy in release/latest/:
  livegomezsynth-<version>-smk37.fwsc   the package
  manifest.json                         identity, size, SHA-256, budgets, catalog and test facts, hardware status
  SHA256SUMS.txt                        every file of the release
  RELEASE-NOTES.md, INSTALL.md          generated notes, the install guide (DOCs/install-guide.md)
  THIRD_PARTY.md, LICENSE, LICENSES/    licence files
  site/                                 the simulator at / and the installer at /install/ (upload as a static site: HTTPS needed)
Checks first (a failure stops the release): name review, level/golden test, installer test, package identity and structure.
"""
import argparse, hashlib, json, os, pathlib, re, shutil, subprocess, sys, datetime

root = pathlib.Path(__file__).resolve().parent.parent
BLK, KEEP = 0x30, 0x2F
sha256 = lambda b: hashlib.sha256(b).hexdigest()


def product_of(fwsc):
    """the package identity: one marker byte after each 0x2F-byte block (0x7D = none), as web/install/package.js"""
    s = ""
    for i in range(20):
        m = fwsc[i * BLK + KEEP]
        if m != 0x7D:
            s += chr((m - i - 1) & 0xFF)
    return s


def run(cmd, **kw):
    r = subprocess.run(cmd, cwd=root, capture_output=True, text=True, **kw)
    return r.returncode, r.stdout + r.stderr


def gate(title, cmd):
    code, out = run(cmd)
    print("  %-4s %s" % ("ok" if code == 0 else "FAIL", title))
    if code:
        print(out[-1500:])
        raise SystemExit("release: %s failed" % title)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default=None, help="default: src/VERSION")
    ap.add_argument("--bump", action="store_true", help="raise the last number of src/VERSION first (once per commit)")
    ap.add_argument("--id", default="20000")
    ap.add_argument("--build", action="store_true", help="run lgs_build.py first")
    a = ap.parse_args()
    vfile = root / "VERSION"
    if a.bump:
        parts = vfile.read_text().strip().split(".")
        parts[-1] = str(int(parts[-1]) + 1)
        vfile.write_text(".".join(parts) + "\n")
    a.version = a.version or vfile.read_text().strip()
    print("version  " + a.version)
    budget = None
    if a.build:
        out = gate("device package build", [sys.executable, "tools/lgs_build.py", "--version", a.version, "--id", a.id])
        m = re.search(r"image (\d+) B of (\d+); RAM .data\+.bss (\d+) B of (\d+)", out)
        budget = dict(zip(("image", "image_max", "ram", "ram_max"), map(int, m.groups()))) if m else None
    pkg_path = root / "build" / ("livegomezsynth-%s-smk37.fwsc" % a.version)
    if not pkg_path.exists():
        raise SystemExit("no package at %s (run with --build)" % pkg_path)
    pkg = pkg_path.read_bytes()
    identity = product_of(pkg)
    if identity != "SMK-37 Pro_" + a.id:
        raise SystemExit("package identity %r, expected SMK-37 Pro_%s" % (identity, a.id))

    print("checks")
    gate("simulator build", ["sh", "web/sim/build.sh"])
    gate("name review (D-42)", [sys.executable, "tools/name_review.py"])
    gate("catalog levels and golden renders", ["node", "tests/catalog_health.mjs"])
    gate("installer against a fake keyboard", ["node", "tests/installer_test.mjs"])
    for f in ("THIRD_PARTY.md", "LICENSE"):
        if not (root / f).exists():
            raise SystemExit("missing " + f)
    tp = (root / "THIRD_PARTY.md").read_text()
    for need in ("msfa", "DaisySP", "Clipper", "SMKSYNTH", "CC0", "JieLi"):
        if need.lower() not in tp.lower():
            raise SystemExit("THIRD_PARTY.md does not mention " + need)
    print("  ok   licence files and third-party notices")

    cat = json.load(open(root / "catalog" / "catalog.json"))
    per_cat = {}
    per_eng = {}
    for p in cat["patches"]:
        per_cat[cat["categories"][p["cat"] - 1]] = per_cat.get(cat["categories"][p["cat"] - 1], 0) + 1
        per_eng[p["engine"]] = per_eng.get(p["engine"], 0) + 1
    code, rev = run(["git", "rev-parse", "HEAD"])
    code2, dirty = run(["git", "status", "--porcelain", "--", "."])
    wasm = (root / "build" / "sim-site" / "lgs.wasm").read_bytes()
    golden = (root / "catalog" / "golden.json").read_bytes()
    name = pkg_path.name
    out = root / "release" / a.version
    if out.exists():
        shutil.rmtree(out)
    (out / "site" / "install" / "firmware").mkdir(parents=True)
    shutil.copy(pkg_path, out / name)
    manifest = {
        "name": "LiveGomezSynth " + a.version,
        "version": a.version,
        "product": identity,
        "family": "LIVEGMZ",
        "file": name,
        "size": len(pkg),
        "sha256": sha256(pkg),
        "built": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "source_commit": rev.strip() if code == 0 else None,
        "source_dirty": bool(dirty.strip()) if code2 == 0 else None,
        "package_kind": "update (needs SMKSYNTH 1.x or LiveGomezSynth on the keyboard; no first-install package for stock V016 yet)",
        "budgets": budget,
        "patches": len(cat["patches"]),
        "patches_per_category": per_cat,
        "patches_per_engine": per_eng,
        "golden_sha256": sha256(golden),
        "simulator_wasm_sha256": sha256(wasm),
        "hardware_tested": False,
        "gates": {"G1_simulator_owner_validation": "pending", "G2_hardware_dev": "pending", "G3_soak": "pending"},
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    # the site: simulator at /, installer at /install/
    shutil.copytree(root / "build" / "sim-site", out / "site", dirs_exist_ok=True)
    (out / "site" / "version.json").write_text(json.dumps({k: manifest[k] for k in ("version", "built", "size", "file")}) + "\n")
    for f in ("index.html", "app.js", "service.js", "protocol.js", "package.js", "style.css", "JieLi-Apache-2.0.txt"):
        shutil.copy(root / "web" / "install" / f, out / "site" / "install" / f)
    shutil.copy(pkg_path, out / "site" / "install" / "firmware" / name)
    pin = {k: manifest[k] for k in ("name", "product", "size", "sha256", "version")}
    pin["url"] = "./firmware/" + name
    (out / "site" / "install" / "manifest.json").write_text(json.dumps(pin, indent=2) + "\n")
    for f in ("THIRD_PARTY.md", "LICENSE"):
        shutil.copy(root / f, out / f)
    shutil.copytree(root / "LICENSES", out / "LICENSES")
    shutil.copy(root.parent / "DOCs" / "install-guide.md", out / "INSTALL.md")
    notes = ["# LiveGomezSynth %s" % a.version, "",
             "Custom live-performance firmware for the M-VAVE SMK-37 Pro. Identity `%s`, family `LIVEGMZ`." % identity, "",
             "**Status: not tested on hardware.** The simulator build is the reference; gates G2 (first install) and G3 (soak) are pending.", "",
             "## Contents", "", "- %d patches in %d categories (%s)" % (len(cat["patches"]), len(per_cat), ", ".join("%s %d" % kv for kv in per_cat.items())),
             "- engines: " + ", ".join("%s %d" % kv for kv in sorted(per_eng.items())),
             "- package: `%s`, %d bytes, SHA-256 `%s`" % (name, len(pkg), sha256(pkg))]
    if budget:
        notes.append("- budgets: image %d of %d bytes, RAM %d of %d bytes" % (budget["image"], budget["image_max"], budget["ram"], budget["ram_max"]))
    notes += ["", "## Before installing", "", "Read INSTALL.md. Back up first: the installer page saves an identity record and your current firmware package; the first start re-initializes the user-data bank.", "",
              "## Known limits", "", "- Sounds are first drafts built from parameters; they were level-matched by measurement, not judged by ear.",
              "- Update package only: stock M-VAVE V016 keyboards need SMKSYNTH or a first-install package first.",
              "- The installer's restore path is untested on hardware; M-UPGRADE with the factory package is the official way back."]
    (out / "RELEASE-NOTES.md").write_text("\n".join(notes) + "\n")
    sums = []
    for f in sorted(p for p in out.rglob("*") if p.is_file() and p.name != "SHA256SUMS.txt"):
        sums.append("%s  %s" % (sha256(f.read_bytes()), f.relative_to(out)))
    (out / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n")
    latest = root / "release" / "latest"
    if latest.exists():
        shutil.rmtree(latest)
    shutil.copytree(out, latest)
    print("release  %s  (%d files)" % (out, len(sums)))
    print("sha256   %s" % manifest["sha256"])
    print("publish  release/latest/site  (static hosting over HTTPS: the installer needs Web MIDI)")


if __name__ == "__main__":
    main()
