#!/usr/bin/env python3
"""Build the watchface. Production by default (copied to dist/); fixture switches build into
watchface/build only and must never be distributed.

  python3 execution/build.py                    # production → dist/straight-ahead.pbw
  python3 execution/build.py --heading 45       # freeze the heading (emulator has no compass)
  python3 execution/build.py --minute 720      # freeze the day ring at noon
  python3 execution/build.py --debug            # raw vector + accumulation readout
  python3 execution/build.py --dry-run          # print what would run
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WATCH = ROOT / "watchface"
DIST = ROOT / "dist" / "straight-ahead.pbw"


def build(heading: int | None = None, debug: bool = False, dry_run: bool = False,
          minute: int | None = None) -> Path:
    env = os.environ.copy()
    for name in ("SA_DEBUG", "SA_TEST_HEADING", "SA_TEST_MINUTE"):
        env.pop(name, None)
    if heading is not None:
        env["SA_TEST_HEADING"] = str(heading)
    if minute is not None:
        env["SA_TEST_MINUTE"] = str(minute)
    if debug:
        env["SA_DEBUG"] = "1"
    fixture = heading is not None or minute is not None or debug
    label = f"heading={heading} minute={minute} debug={int(debug)}" if fixture else "production"
    if dry_run:
        print(f"would build {label} in {WATCH.relative_to(ROOT)}")
        return WATCH / "build" / "watchface.pbw"
    # `pebble clean` so a fixture build can never leak into dist/ via cached objects.
    subprocess.run(["pebble", "clean"], cwd=WATCH, env=env, check=True, capture_output=True)
    result = subprocess.run(["pebble", "build"], cwd=WATCH, env=env, capture_output=True, text=True)
    (ROOT / ".tmp").mkdir(exist_ok=True)
    (ROOT / ".tmp" / "build.log").write_text(result.stdout + result.stderr)
    if result.returncode != 0:
        raise SystemExit(f"pebble build failed ({label}); see .tmp/build.log")
    pbw = WATCH / "build" / "watchface.pbw"
    if not fixture:
        DIST.parent.mkdir(exist_ok=True)
        shutil.copyfile(pbw, DIST)
        print(f"built {DIST.relative_to(ROOT)}")
    else:
        print(f"built fixture ({label}) in watchface/build only; not for distribution")
    return pbw


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--heading", type=int, help="freeze the heading 0..359 (fixture)")
    ap.add_argument("--minute", type=int, help="freeze minutes since midnight 0..1439 (fixture)")
    ap.add_argument("--debug", action="store_true", help="build with SA_DEBUG=1 (fixture)")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()
    build(a.heading, a.debug, a.dry_run, a.minute)
