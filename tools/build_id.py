#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# What a build was made from: the version string, the git commit and whether the sources had uncommitted changes.
# tools/build.py writes it to build/build.json, web/emu/build.sh to build/emu/build.json (published with the
# emulator); tools/check_emu.py compares them.
#   tools/build_id.py [VERSION]   ->  {"version": ..., "commit": ..., "dirty": ...} on stdout
import json
import re
import subprocess
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent
SOURCES = ["firmware", "web", "tools", "assets", "build.sh"]     # what the firmware and the emulator are built from


def git(*args):
    try:
        r = subprocess.run(["git", *args], cwd=SRC, capture_output=True, text=True)
    except OSError:
        return None
    return r.stdout.strip() if r.returncode == 0 else None


def build_id(version=None):
    if version is None:
        m = re.search(r'^#define FELUCCA_VERSION "([^"]*)"', (SRC / "firmware/src/felucca.c").read_text(), re.M)
        version = m[1] if m else "?"
    commit = git("rev-parse", "HEAD")
    status = git("status", "--porcelain", "--", *SOURCES)
    return {"version": version, "commit": commit or "unknown", "dirty": bool(status)}


def label(b):
    """e.g. 3eafae6 or 3eafae6-dirty"""
    return b["commit"][:7] + ("-dirty" if b.get("dirty") else "")


if __name__ == "__main__":
    print(json.dumps(build_id(sys.argv[1] if len(sys.argv) > 1 else None)))
