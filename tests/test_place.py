"""Contract D1-D3, D6-D9, D14 (host): the watch's integer ring and label rules, compiled from
watchface/src/c/place.c. D6 uses Spring, TX city records from the real phone code (node)."""
from __future__ import annotations

import json
import math
import re
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "watchface" / "src"
HARNESS = ROOT / ".tmp" / "place_harness"
DISC_R = 84

pytestmark = pytest.mark.skipif(shutil.which("cc") is None, reason="needs a C compiler")


@pytest.fixture(scope="module", autouse=True)
def harness():
    HARNESS.parent.mkdir(exist_ok=True)
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", f"-I{SRC / 'c'}", "-o", str(HARNESS),
                    str(ROOT / "tests/host/place_harness.c"), str(SRC / "c/place.c")], check=True)


def call(*args: str, stdin: str = "") -> int:
    out = subprocess.run([str(HARNESS), *args], input=stdin, capture_output=True, text=True, check=True)
    return int(out.stdout)


def records(rows: list[tuple[int, int, str]]) -> str:
    return "".join(f"{nm} {b10} {name}\n" for nm, b10, name in rows)


@pytest.mark.parametrize("minute", [0, 1, 59, 360, 720, 1080, 1439])
def test_d1_ring_radius(minute):
    assert abs(call("ring", str(minute), str(DISC_R)) - minute * DISC_R / 1440) <= 1


def test_d2_ring_is_zero_at_midnight():
    assert call("ring", "0", str(DISC_R)) == 0


def test_d3_ring_reaches_rim_at_2359():
    assert call("ring", "1439", str(DISC_R)) in (DISC_R - 1, DISC_R)


def test_d7_city_outside_bearing_window_never_picked():
    assert call("pick", "720", "0", stdin=records([(720, 121, "EAST OF WINDOW")])) == -1
    assert call("pick", "720", "0", stdin=records([(720, 3479, "WEST OF WINDOW")])) == -1
    assert call("pick", "720", "0", stdin=records([(720, 120, "EDGE")])) == 0


def test_d8_city_outside_distance_window_never_picked():
    assert call("pick", "720", "0", stdin=records([(746, 0, "BEYOND")])) == -1
    assert call("pick", "720", "0", stdin=records([(694, 0, "SHORT")])) == -1
    assert call("pick", "720", "0", stdin=records([(745, 0, "EDGE")])) == 0


def test_window_wraps_through_north():
    # Heading 355 degrees, city at 3 degrees: 8 degrees apart, inside the window.
    assert call("pick", "720", "3550", stdin=records([(720, 30, "ACROSS NORTH")])) == 0


def test_pick_prefers_smallest_combined_miss():
    rows = [(740, 0, "RADIAL 20"), (720, 50, "LATERAL ~63"), (725, 10, "BEST")]
    assert call("pick", "720", "0", stdin=records(rows)) == 2


@pytest.mark.parametrize("minute,heading10,cell", [
    (0, 0, 0), (59, 0, 0), (60, 0, 1), (1439, 0, 23),
    (720, 900, 9 * 24 + 12), (720, 3549, 35 * 24 + 12), (720, 3550, 0 * 24 + 12), (720, 49, 12),
])
def test_d9_region_cell(minute, heading10, cell):
    assert call("cell", str(minute), str(heading10)) == cell


def spring_cities() -> list[list]:
    script = ("const geo=require('./src/pkjs/geo.js'),g=require('./src/pkjs/geodata.js');"
              "console.log(JSON.stringify(geo.nearbyCities(g,30.08,-95.42,160)))")
    out = subprocess.run(["node", "-e", script], cwd=ROOT / "watchface", capture_output=True, text=True, check=True)
    return json.loads(out.stdout)


@pytest.mark.skipif(shutil.which("node") is None, reason="needs node")
def test_d6_spring_noon_north_names_a_city_in_the_window():
    cities = spring_cities()
    idx = call("pick", "720", "0", stdin=records([tuple(c) for c in cities]))
    assert idx >= 0, "expected a city; regional fallback would also satisfy D6 but data has one"
    nm, b10, name = cities[idx]
    assert abs(nm - 720) <= 25
    assert min(b10, 3600 - b10) <= 120
    assert name == "DES MOINES"


def test_d14_c_sources_have_no_float():
    for path in (SRC / "c").glob("*.[ch]"):
        text = re.sub(r"//.*", "", path.read_text())
        assert not re.search(r"\b(float|double)\b", text), path.name
