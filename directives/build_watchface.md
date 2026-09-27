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
