#!/usr/bin/env python3
"""Bundle Natural Earth data for the phone side of Straight Ahead.

Writes watchface/src/pkjs/geodata.js (CommonJS) with:
  cities      major cities [name, lat, lon], largest first: pop_max >= MIN_POP or a national capital
  coast       Natural Earth 1:110m coastline (the phone projects it for both views, then thins)
  names       region names; index 0 = unnamed
  grid        0.5-degree "what's here" raster, run-length encoded per row as
              [id, run, id, run, ...]; seas first, then countries, then US states on top

Downloads are cached in .tmp/ne/. Requires Pillow.

  python3 execution/build_geodata.py            # write the bundle
  python3 execution/build_geodata.py --dry-run  # print stats only
"""
from __future__ import annotations

import argparse
import json
import unicodedata
from pathlib import Path
from urllib.request import urlopen

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".tmp" / "ne"
OUT = ROOT / "watchface" / "src" / "pkjs" / "geodata.js"
NE_BASE = "https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson"

MIN_POP = 300_000
CELL = 0.5  # degrees
GRID_W, GRID_H = int(360 / CELL), int(180 / CELL)
MAX_NAME = 22
MAX_BUNDLE_BYTES = 400_000  # contract D10

LAYERS = {
    "coast": "ne_110m_coastline.geojson",
    "places": "ne_50m_populated_places_simple.geojson",
    "marine": "ne_50m_geography_marine_polys.geojson",
    "countries": "ne_110m_admin_0_countries.geojson",
    "states": "ne_110m_admin_1_states_provinces.geojson",
}


def fetch(name: str) -> list[dict]:
    path = CACHE / name
    if not path.exists():
        CACHE.mkdir(parents=True, exist_ok=True)
        with urlopen(f"{NE_BASE}/{name}", timeout=60) as response:
            path.write_bytes(response.read())
    return json.loads(path.read_text())["features"]


def label(text: str) -> str:
    ascii_text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode()
    return " ".join(ascii_text.upper().split())[:MAX_NAME]


def prop(feature: dict, *keys: str):
    props = feature["properties"]
    for k in keys:
        if props.get(k) not in (None, ""):
            return props[k]
    return None


def lines(geom: dict):
    t, c = geom["type"], geom["coordinates"]
    if t == "LineString":
        yield c
    elif t == "MultiLineString":
        yield from c


def rings(geom: dict):
    t, c = geom["type"], geom["coordinates"]
    if t == "Polygon":
        yield c
    elif t == "MultiPolygon":
        yield from c


def round_line(line, nd=2):
    return [[round(x, nd), round(y, nd)] for x, y in line]


def build_grid(marine, countries, states):
    names = [""]
    index: dict[str, int] = {}

    def name_id(n: str) -> int:
        if n not in index:
            index[n] = len(names)
            names.append(n)
        return index[n]

    # "I" mode: 32-bit ids, so no palette limit while painting.
    img = Image.new("I", (GRID_W, GRID_H), 0)
    draw = ImageDraw.Draw(img)

    def to_px(lon, lat):
        return ((lon + 180) / CELL, (90 - lat) / CELL)

    def paint(features, keys):
        for f in features:
            n = prop(f, *keys)
            if not n:
                continue
            fill = name_id(label(n))
            for poly in rings(f["geometry"]):
                outer = [to_px(x, y) for x, y in poly[0]]
                if len(outer) >= 3:
                    draw.polygon(outer, fill=fill)

    paint(marine, ("name_en", "name", "NAME"))
    paint(countries, ("NAME_EN", "name_en", "NAME", "name", "ADMIN"))
    paint(states, ("name_en", "name", "NAME"))

    px = img.load()
    grid = []
    for y in range(GRID_H):
        row, run_id, run = [], px[0, y], 0
        for x in range(GRID_W):
            v = px[x, y]
            if v == run_id:
                run += 1
            else:
                row += [run_id, run]
                run_id, run = v, 1
        row += [run_id, run]
        grid.append(row)
    return names, grid


def build() -> dict:
    coast = [round_line(ln) for f in fetch(LAYERS["coast"]) for ln in lines(f["geometry"])]

    cities = []
    for f in fetch(LAYERS["places"]):
        pop = prop(f, "pop_max", "POP_MAX") or 0
        cap = prop(f, "adm0cap", "ADM0CAP") or 0
        if pop >= MIN_POP or cap:
            lon, lat = f["geometry"]["coordinates"][:2]
            cities.append((pop, [label(prop(f, "name", "NAME")), round(lat, 3), round(lon, 3)]))
    # Largest first: the phone keeps the first N in range, so a crowded region loses its small
    # cities, never its far ones.
    cities = [c for _, c in sorted(cities, key=lambda pc: -pc[0])]

    names, grid = build_grid(fetch(LAYERS["marine"]), fetch(LAYERS["countries"]), fetch(LAYERS["states"]))
    return {"cell": CELL, "cities": cities, "coast": coast, "names": names, "grid": grid}


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dry-run", action="store_true", help="print stats, write nothing")
    args = ap.parse_args()

    data = build()
    body = "// GENERATED by execution/build_geodata.py from Natural Earth (public domain). Do not edit.\n"
    body += "module.exports = " + json.dumps(data, separators=(",", ":")) + ";\n"
    npts = sum(len(ln) for ln in data["coast"])
    print(f"cities={len(data['cities'])} coast={npts} pts names={len(data['names'])} "
          f"bundle={len(body) / 1000:.0f} KB")
    if len(body) > MAX_BUNDLE_BYTES:
        raise SystemExit(f"bundle {len(body)} B exceeds {MAX_BUNDLE_BYTES} B (contract D10)")
    if args.dry_run:
        return
    OUT.write_text(body)
    print(f"wrote {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
