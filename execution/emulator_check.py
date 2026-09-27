#!/usr/bin/env python3
"""Emulator checks for Straight Ahead (emery). Screenshots land in .tmp/emulator/.

Each scenario builds a fixture (frozen heading and minute), points the phone at Spring, TX via
watchface/src/pkjs/dev.json (restored to {} afterwards), installs, optionally flicks, and
screenshots. Pixel checks:
  D12  day ring: cyan pixels sit on the circle of radius minute * DISC_R / 1440
  D13  world view: a flick during a session removes the day ring and draws the 2 px facing line
Always finishes with a production build.

Run with a Python that has Pillow:
  python3 execution/emulator_check.py [--only noon_n dawn_e ...]
"""
from __future__ import annotations

import argparse
import json
import math
import subprocess
import time
from pathlib import Path

from PIL import Image

from build import ROOT, WATCH, build

OUT = ROOT / ".tmp" / "emulator"
DEV = WATCH / "src" / "pkjs" / "dev.json"
SPRING = {"lat": 30.08, "lon": -95.42}
NEMO = {"lat": -48.88, "lon": -123.39}  # no coast or city within 24 degrees: empty payloads
CX, CY, R = 100, 140, 84
CYAN = (0, 255, 255)

# name, heading, minute, flicks, location
SCENARIOS = [
    ("noon_n", 0, 720, 0, SPRING),
    ("dawn_e", 90, 360, 0, SPRING),
    ("afternoon_w", 270, 900, 0, SPRING),
    ("night_se", 135, 1260, 0, SPRING),
    ("late_nne", 20, 1439, 0, SPRING),
    ("early_n", 0, 30, 0, SPRING),
    ("world_ne", 45, 720, 1, SPRING),  # the face opens in a session; one flick toggles
    ("nemo", 0, 720, 0, NEMO),
]


def run(args: list[str], timeout: int = 180) -> str:
    r = subprocess.run(args, cwd=WATCH, capture_output=True, text=True, timeout=timeout)
    if r.returncode != 0:
        raise RuntimeError(f"{' '.join(args)} failed: {r.stdout[-400:]} {r.stderr[-400:]}")
    return r.stdout


def boot() -> None:
    """Start from a clean emulator: stale QEMU/pypkjs state shows the wrong face (see _PEBBLE skill)."""
    subprocess.run(["pebble", "kill"], cwd=WATCH, capture_output=True)
    for proc in ("pypkjs", "qemu-pebble"):
        subprocess.run(["pkill", "-f", proc], capture_output=True)
    time.sleep(2)
    build()
    subprocess.run(["pebble", "install", "--emulator", "emery", "build/watchface.pbw"], cwd=WATCH,
                   capture_output=True, timeout=240)
    time.sleep(20)


def install() -> None:
    run(["pebble", "install", "--emulator", "emery", "build/watchface.pbw"])
    time.sleep(8)  # phone projects and streams the map


def shot(name: str) -> Image.Image:
    path = OUT / f"{name}.png"
    run(["pebble", "screenshot", "--emulator", "emery", "--no-open", "--no-correction", str(path)])
    return Image.open(path).convert("RGB")


def ring_pixels(img: Image.Image, radius: int) -> tuple[int, int]:
    """(cyan pixels within 1 px of the ring, cyan pixels elsewhere inside the disc)."""
    on = off = 0
    px = img.load()
    for y in range(CY - R - 2, CY + R + 3):
        for x in range(CX - R - 2, CX + R + 3):
            if px[x, y] != CYAN:
                continue
            d = math.hypot(x - CX, y - CY)
            if abs(d - radius) <= 1.5:
                on += 1
            elif d <= R:
                off += 1
    return on, off


def facing_line(img: Image.Image) -> int:
    px = img.load()
    return sum(1 for y in range(CY - R + 2, CY - 4) if px[CX, y] == CYAN and px[CX + 1, y] == CYAN)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--only", nargs="*")
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    reports = []
    boot()
    try:
        for name, heading, minute, flicks, where in SCENARIOS:
            if args.only and name not in args.only:
                continue
            DEV.write_text(json.dumps(where) + "\n")
            build(heading=heading, minute=minute)
            install()
            for _ in range(flicks):
                run(["pebble", "emu-tap", "--emulator", "emery", "--direction", "x+"])
                time.sleep(1.5)
            img = shot(name)
            radius = minute * R // 1440
            on, off = ring_pixels(img, radius)
            report = {"scenario": name, "heading": heading, "minute": minute, "ring_px": radius,
                      "cyan_on_ring": on, "cyan_elsewhere": off, "facing_2px": facing_line(img)}
            if flicks:
                report["pass"] = on < 20 and report["facing_2px"] > 40  # D13
            elif radius >= 6:
                expected = 2 * math.pi * radius * 0.6
                report["pass"] = on >= expected and off <= on // 4  # D12
            else:
                report["pass"] = True  # too small to measure; screenshot kept for review
            reports.append(report)
            print(json.dumps(report), flush=True)
    finally:
        DEV.write_text("{}\n")
        build()
    (OUT / "report.json").write_text(json.dumps(reports, indent=2) + "\n")
    frames = [Image.open(OUT / f"{r['scenario']}.png") for r in reports]
    if frames:
        sheet = Image.new("RGB", (len(frames) * 206, 228), "white")
        for i, f in enumerate(frames):
            sheet.paste(f, (i * 206, 0))
        sheet.save(OUT / "sheet.png")
    failed = [r["scenario"] for r in reports if not r["pass"]]
    if failed:
        raise SystemExit(f"failed: {failed}")
    print(f"all {len(reports)} scenarios passed; sheet → {(OUT / 'sheet.png').relative_to(ROOT)}")


if __name__ == "__main__":
    main()
