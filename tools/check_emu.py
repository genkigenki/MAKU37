#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Is the emulator on GitHub Pages built from the same commit as the local build?
#   tools/check_emu.py [--url URL]
# Compares the site's build.json with build/build.json (the firmware, ./build.sh) and build/emu/build.json
# (the local emulator, web/emu/build.sh). Exit 0: the local firmware build and the site match; 1: they differ;
# 2: something is missing.
import argparse
import json
import subprocess
import sys
import time
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_id import SRC, build_id, label  # noqa: E402

URL = "https://okumurari2.github.io/maku/build.json"


def load(path):
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError):
        return None


def relation(remote, local):
    """how the site's commit stands to a local one, when git knows both"""
    if remote == local:
        return "same commit"
    def anc(a, b):
        return subprocess.run(["git", "merge-base", "--is-ancestor", a, b], cwd=SRC,
                              capture_output=True).returncode == 0
    if anc(remote, local):
        return "site is older (not deployed yet?)"
    if anc(local, remote):
        return "site is newer"
    return "different commits"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--url", default=URL, help=f"the site's build.json (default {URL})")
    a = ap.parse_args()
    try:                            # Pages caches for minutes: ask for a fresh copy
        with urllib.request.urlopen(f"{a.url}?t={int(time.time())}", timeout=15) as r:
            site = json.load(r)
    except Exception as e:  # noqa: BLE001
        print(f"site:     cannot read {a.url} ({e}); the deployed emulator may predate build.json")
        return 2
    head = build_id()
    rows = [("site", site), ("firmware", load(SRC / "build/build.json")),
            ("emu", load(SRC / "build/emu/build.json")), ("HEAD", head)]
    for name, b in rows:
        if b is None:
            print(f"{name + ':':<10}(none)")
            continue
        note = ""
        if b is not site:
            note = f"  {relation(site['commit'], b['commit'])}" + (" + uncommitted changes" if b.get("dirty") else "")
        print(f"{name + ':':<10}{b['version']:<10}{label(b)}{note}")
    fw = rows[1][1]
    if fw is None:
        print("no build/build.json: run ./build.sh")
        return 2
    same = fw["commit"] == site["commit"] and not fw.get("dirty")
    print("MATCH" if same else "DIFFERENT")
    return 0 if same else 1


if __name__ == "__main__":
    sys.exit(main())
