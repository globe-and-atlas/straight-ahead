---
generated_by: "Claude Code CLI (claude-opus-5-5)"
timestamp: "2026-09-27T10:55:00-05:00"
---

# Directive: build_watchface

## Goal

Ship "Straight Ahead", a Pebble Time 2 (emery) watch face: an azimuthal-equidistant world centred
on the wearer and turned to their heading. In that projection every great circle through the
centre is a straight line, so the vertical is the great circle you face; it is labelled with its
first landfall. The rim is your antipode. Novelty check (2026-09-27): no shipped face combines a
wearer-centred azimuthal-equidistant map with compass heading — see
`../reports/Cartographic watch face prior art.md` (Addendum).

## Architecture

| Layer | Does | Why there |
|---|---|---|
| Phone (`src/pkjs`) | location, AEQD projection of coastlines, landfall per 5° bearing, magnetic declination | trig-heavy, runs a few times a day |
| Watch (`src/c`) | rotate received points by heading, draw, compass sessions | integer `sin_lookup` only |
| `execution/` | coastline preparation, builds, emulator checks | deterministic, repeatable |

## Inputs

- Phone location (PebbleKit JS geolocation), Natural Earth 1:110m coastline and admin-0 countries,
  WMM2025 coefficients, compass (magnetic heading)

## Outputs

- `dist/straight-ahead.pbw`; emulator screenshots in `.tmp/emulator/`

## Validation Contract

Binary pass/fail. Rows marked (M0) hold for the scaffold; the rest define later milestones.

| # | Assertion | Check |
|---|---|---|
| V1 (M0) | `pebble build` for emery exits 0 | `execution/build.py` |
| V2 (M0) | With no compass session, the header reads `N-UP` and north is up | emulator screenshot |
| V3 (M0) | With heading fixture 45, the north tick sits at -45° (upper left) | `execution/build.py --heading 45` + screenshot |
| V4 (M0) | A compass status below Calibrating never shows a numeric heading | code review + emulator (`NO CMPS`) |
| V5 (M0) | The compass is subscribed only during a flick-started session | code review (`heading.c`) |
| V6 (M0) | A session ends 60 s after the last flick | code review; M1 on-watch timing |
| V7 (M1) | Heading error is ≤ 15° against 4 surveyed bearings after calibration | on-watch log |
| V8 (M1) | Battery drain with 20 sessions/day is ≤ 5 %/day more than north-up only | 3-day on-watch trial |
| V9 (M2) | A projected point's radius equals great-circle distance / 20,015 km × disc radius (±1 px) | host test vs pyproj/geographiclib |
| V10 (M2) | A point's screen bearing equals its initial great-circle bearing minus heading (±2°) | host test |
| V11 (M2) | Coastline payload fits the watch's budget (≤ 1,600 points) | host test |
| V12 (M3) | Landfall for Spring TX at 90° names the expected first landfall | host test with fixture location |
| V13 (M4) | True heading = magnetic heading + WMM declination (±0.5°) | host test vs NOAA calculator values |
| V14 | CloudPebble import layout holds (only .c/.h in src/c, .js/.json in src/pkjs, no required -D) | host test |

## Edge Cases

- No location yet: hollow centre dot, `NO FIX`, map stays empty.
- Compass unavailable / invalid: `NO CMPS` / `HDG ...`, map stays north-up.
- Wearer near a pole or the antimeridian: the projection is centred on the wearer, so neither is special — test anyway.

## Learnings

### 2026-09-27
- The emery emulator reports `CompassStatusUnavailable`; `pebble emu-compass` does nothing on it. Rotation is tested with the `SA_TEST_HEADING` fixture; the real compass needs the watch (M1).

## Addendum 2026-09-27 — Day ring (option C)

Two views. **Day ring** (default): a regional azimuthal-equidistant map whose rim is 24° (1,440 nmi)
from the wearer. A ring sits at *minutes since local midnight* nautical miles (1 arcminute of great
circle = 1 nmi), so it grows from the wearer at 00:00 to the rim at 23:59. The vertical is still the
bearing faced (north when north-up). The label names what lies where the ring crosses that bearing:
the nearest major city in a window, otherwise the water body, US state or country there.
**World** view: the whole-Earth disc from M0. A flick during a compass session toggles views; the
session ending returns to the day ring, north-up. The distance rule is shared with From Here
(H°M′ = minutes since midnight in arcminutes); what's new here is the compass bearing, the drawn
ring and the named place.

Label rule (watch, integer): among cities with |d − m| ≤ 25 nmi and |bearing − heading| ≤ 12°,
pick the smallest |d − m| + d·|Δθ|·π/180 (lateral miss). If none, use the region cell at
(round(heading / 10°), m / 60): 36 bearings × 24 distance bands, filled by the phone.

| # | Assertion | Check |
|---|---|---|
| D1 | Ring radius px = minutes × DISC_R / 1440 (±1 px) | host test (C) |
| D2 | At 00:00 the ring radius is 0 | host test |
| D3 | At 23:59 the ring radius is DISC_R − 1 or DISC_R | host test |
| D4 | Phone AEQD: projected radius = great-circle distance / scale × 127 (±1 unit) vs an independent Python implementation | node + python test |
| D5 | Phone AEQD: projected bearing within 1° of the initial great-circle bearing | node + python test |
| D6 | Spring TX, 12:00, heading 0 → label OMAHA or a city within the window (Omaha is 671 nmi at 358°; window is 695–745 nmi, so a regional fallback such as IOWA is also correct) | host test with fixture data |
| D7 | A city outside ±12° is never picked | host test |
| D8 | A city outside ±25 nmi of the ring is never picked | host test |
| D9 | Region fallback returns the cell for the rounded bearing and distance band | host test |
| D10 | Geodata bundle for pkjs is ≤ 400 KB | build script check |
| D11 | Coastline payload per view is ≤ 1,500 points | node test |
| D12 | Emulator: day-ring screenshot at a fixture minute shows the cyan ring at the expected radius | emulator check |
| D13 | Emulator: world view is reachable with a flick during a session | emulator check (fixture heading) |
