"""Contract D4, D5, D10, D11 and V14: the phone's azimuthal-equidistant projection (node) checked
against an independent vector-based implementation in Python, plus payload budgets and the
CloudPebble import layout."""
from __future__ import annotations

import json
import math
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
WATCH = ROOT / "watchface"
R_NM = 10800 / math.pi

pytestmark = pytest.mark.skipif(shutil.which("node") is None, reason="needs node")

CENTRES = [(30.08, -95.42), (51.5, -0.12), (-33.87, 151.21), (64.1, -21.9), (0.0, 179.9)]
TARGETS = [(41.58, -93.62), (29.95, -90.07), (-12.05, -77.04), (35.68, 139.69), (60.0, -85.0),
           (-54.8, -68.3), (1.29, 103.85), (30.5, -95.0),
           # one within the 24-degree day disc of each centre
           (48.85, 2.35), (-37.81, 144.96), (55.68, 12.57), (-18.14, 178.44)]


def node(script: str):
    out = subprocess.run(["node", "-e", script], cwd=WATCH, capture_output=True, text=True, check=True)
    return json.loads(out.stdout)


def unit(lat, lon):
    p, l = math.radians(lat), math.radians(lon)
    return (math.cos(p) * math.cos(l), math.cos(p) * math.sin(l), math.sin(p))


def reference(lat1, lon1, lat2, lon2):
    """Distance via the angle between unit vectors; bearing via the local east/north frame."""
    a, b = unit(lat1, lon1), unit(lat2, lon2)
    cross = (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
    ang = math.atan2(math.sqrt(sum(c * c for c in cross)), sum(x * y for x, y in zip(a, b)))
    p, l = math.radians(lat1), math.radians(lon1)
    east = (-math.sin(l), math.cos(l), 0.0)
    north = (-math.sin(p) * math.cos(l), -math.sin(p) * math.sin(l), math.cos(p))
    brg = math.degrees(math.atan2(sum(x * y for x, y in zip(b, east)), sum(x * y for x, y in zip(b, north)))) % 360
    return ang * R_NM, brg


@pytest.mark.parametrize("scale", [1440, 10800])
def test_d4_d5_projection_matches_reference(scale):
    cases = [(c, t) for c in CENTRES for t in TARGETS]
    script = ("const geo=require('./src/pkjs/geo.js');const cases=%s;"
              "console.log(JSON.stringify(cases.map(([c,t])=>geo.project(c[0],c[1],[[[t[1],t[0]]]],%d).bytes)))"
              % (json.dumps(cases), scale))
    results = node(script)
    checked = 0
    for (c, t), out in zip(cases, results):
        nm, brg = reference(*c, *t)
        if nm > scale:
            assert out == [], f"{c}->{t} beyond the rim must be clipped"
            continue
        assert out[:2] == [-128, -128]
        x, y = out[2], out[3]
        r = math.hypot(x, y)
        assert abs(r - nm / scale * 127) <= 1.0, f"D4 {c}->{t}: r={r:.2f} expected {nm / scale * 127:.2f}"
        if nm / scale * 127 >= 20:  # bearing is only meaningful away from the centre
            got = math.degrees(math.atan2(x, -y)) % 360
            diff = abs((got - brg + 180) % 360 - 180)
            assert diff <= 3.0, f"D5 {c}->{t}: bearing {got:.1f} vs {brg:.1f}"  # int8 rounding
        checked += 1
    assert checked >= (7 if scale == 1440 else 10)  # enough in-disc cases to mean something


def test_d5_inverse_bearing_is_exact():
    cases = [(c, t) for c in CENTRES for t in TARGETS]
    script = ("const geo=require('./src/pkjs/geo.js');const cases=%s;"
              "console.log(JSON.stringify(cases.map(([c,t])=>geo.inverse(c[0],c[1],t[0],t[1]))))" % json.dumps(cases))
    for (c, t), g in zip(cases, node(script)):
        nm, brg = reference(*c, *t)
        assert abs(g["nm"] - nm) < 0.5
        if nm > 1:
            assert abs((g["bearing"] - brg + 180) % 360 - 180) < 1.0


def test_forward_inverts_inverse():
    script = ("const geo=require('./src/pkjs/geo.js');const p=geo.forward(30.08,-95.42,20,1439);"
              "console.log(JSON.stringify(geo.inverse(30.08,-95.42,p.lat,p.lon)))")
    g = node(script)
    assert abs(g["nm"] - 1439) < 0.01 and abs(g["bearing"] - 20) < 0.01


@pytest.mark.parametrize("lat,lon", CENTRES)
def test_d11_coast_payload_within_budget(lat, lon):
    script = ("const geo=require('./src/pkjs/geo.js'),g=require('./src/pkjs/geodata.js');"
              "const f=s=>geo.thin(geo.project(%f,%f,g.coast,s));"
              "console.log(JSON.stringify([f(1440).length,f(10800).length]))" % (lat, lon))
    day, world = node(script)
    # Pen-up pairs are extra; the watch buffer holds 3,600 bytes.
    assert day <= 3600 and world <= 3600


def test_d10_bundle_size():
    assert (WATCH / "src/pkjs/geodata.js").stat().st_size <= 400_000


def test_v14_cloudpebble_layout():
    for path in (WATCH / "src" / "c").iterdir():
        assert path.suffix in (".c", ".h"), path
    for path in (WATCH / "src" / "pkjs").iterdir():
        assert path.suffix in (".js", ".json"), path
    assert json.loads((WATCH / "src/pkjs/dev.json").read_text()) == {}, "dev.json must ship empty"
