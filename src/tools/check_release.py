#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Checks that release/latest is the package of src/VERSION and consistent with itself (manifest, hashes, the copy the
installer serves, version.json). The device package is built locally (JieLi toolchain), so CI can only publish what was
committed: a stale or half-updated release/latest fails here instead of being published.

  python3 tools/check_release.py                 check only
  python3 tools/check_release.py --stage DIR     check, then copy the assets of the GitHub release into DIR
"""
import argparse, hashlib, json, pathlib, shutil, sys

root = pathlib.Path(__file__).resolve().parent.parent
latest = root / "release" / "latest"
STABLE = "livegomezsynth-smk37.fwsc"   # the same bytes under a name that never changes (releases/latest/download/<name>)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def fail(msg):
    print("release check FAILED: " + msg)
    sys.exit(1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage")
    a = ap.parse_args()
    version = (root / "VERSION").read_text().strip()
    man = json.loads((latest / "manifest.json").read_text())
    name = "livegomezsynth-%s-smk37.fwsc" % version
    if man["version"] != version:
        fail("release/latest is version %s, src/VERSION is %s (run package_release.py)" % (man["version"], version))
    if man["file"] != name or not (latest / name).exists():
        fail("package %s missing in release/latest" % name)
    pkg = (latest / name).read_bytes()
    if len(pkg) != man["size"] or sha(pkg) != man["sha256"]:
        fail("package size or SHA-256 differs from manifest.json")
    site = latest / "site"
    pin = json.loads((site / "install" / "manifest.json").read_text())
    served = site / "install" / "firmware" / name
    if pin["version"] != version or pin["sha256"] != man["sha256"] or not served.exists() or served.read_bytes() != pkg:
        fail("the installer's copy of the package (site/install) is not the release package")
    ver = json.loads((site / "version.json").read_text())
    if ver["version"] != version or ver.get("file") != name:
        fail("site/version.json does not match")
    for line in (latest / "SHA256SUMS.txt").read_text().splitlines():
        h, f = line.split("  ", 1)
        if sha((latest / f).read_bytes()) != h:
            fail("SHA256SUMS.txt: %s differs" % f)
    print("release check ok: %s, %d bytes, SHA-256 %s" % (name, len(pkg), man["sha256"]))
    if a.stage:
        out = pathlib.Path(a.stage)
        if out.exists():
            shutil.rmtree(out)
        out.mkdir(parents=True)
        for f in (name, "manifest.json", "RELEASE-NOTES.md", "INSTALL.md"):
            shutil.copy(latest / f, out / f)
        shutil.copy(latest / name, out / STABLE)
        sums = (latest / "SHA256SUMS.txt").read_text().splitlines()
        (out / "SHA256SUMS.txt").write_text("\n".join(sums + ["%s  %s" % (man["sha256"], STABLE)]) + "\n")
        print("staged in %s: %s" % (out, ", ".join(sorted(p.name for p in out.iterdir()))))


if __name__ == "__main__":
    main()
