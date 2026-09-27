"""Contract R1-R4: the wrist-raise detector (watchface/src/c/raise.c) on the host."""
from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "watchface" / "src" / "c"
HARNESS = ROOT / ".tmp" / "raise_harness"

pytestmark = pytest.mark.skipif(shutil.which("cc") is None, reason="needs a C compiler")

DOWN = [0, -100, 50, -200]          # arm hanging: screen vertical
TRANSIT = [-400, -500]              # neither down nor up
UP = [-850, -900, -950, -880]       # looking at the watch


@pytest.fixture(scope="module", autouse=True)
def harness():
    HARNESS.parent.mkdir(exist_ok=True)
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", f"-I{SRC}", "-o", str(HARNESS),
                    str(ROOT / "tests/host/raise_harness.c"), str(SRC / "raise.c")], check=True)


def raises(zs: list[int]) -> int:
    out = subprocess.run([str(HARNESS)], input="\n".join(map(str, zs)) + "\n",
                         capture_output=True, text=True, check=True).stdout.split()
    assert len(out) == len(zs)
    return sum(int(x) for x in out)


def test_r1_down_then_up_is_one_raise():
    assert raises(DOWN + TRANSIT + UP) == 1


def test_r2_up_without_down_is_no_raise():
    assert raises(UP + TRANSIT + UP) == 0


def test_r3_staying_up_is_not_a_second_raise():
    assert raises(DOWN + UP + UP + UP) == 1


def test_r4_two_raises():
    assert raises(DOWN + UP + DOWN + UP) == 2


def test_transit_alone_does_not_arm():
    assert raises(TRANSIT + UP) == 0
