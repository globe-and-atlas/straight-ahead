# Straight Ahead

A Pebble Time 2 (emery) watch face by Globe & Atlas, in azimuthal-equidistant projection centred
on you and turned to the way you face. Two views:

- **Day ring** (default). The disc's rim is 24° (1,440 nautical miles) away. A cyan ring sits
  *minutes since midnight* nautical miles out: one arcminute of great circle is one nautical mile,
  so at 12:00 the ring is 12°00′ away and at 23:59 it touches the rim. The label names what lies
  where the ring crosses the way you face: the nearest major city within ±25 nmi and ±12°,
  otherwise the sea, US state or country there. Faint rings mark 06:00, 12:00 and 18:00.
- **World**. The rim is your antipode, 20,015 km away. Through the centre every great circle is a
  straight line, so the cyan vertical is the great circle you face.

## Use

- **Raise your wrist** (or open the face) and the map turns to your compass heading for 60 s.
  Raising again during that minute extends it. Afterwards the face rests north-up (the day ring
  then names what is due north on the ring). The compass only runs during these sessions.
- **Flick** during a session to toggle the world view (flicks in its first 2 s only extend it, so
  the raise itself never toggles). A flick while north-up starts a session.
- Header: time; `N-UP` / `HDG 045` (`?` while calibrating, `NO CMPS` if unavailable); the ring's
  distance (`12°00'`). `NO FIX` until the phone sends a location.

The day ring shares its distance rule with Globe & Atlas *From Here* (H°M′ = minutes since midnight
in arcminutes); Straight Ahead adds your compass bearing, the drawn ring and the named place.

## Build

```sh
python3 execution/build.py                 # production → dist/straight-ahead.pbw
python3 execution/build.py --heading 45    # fixture: frozen heading (the emery emulator has no compass)
python3 execution/build.py --minute 720    # fixture: frozen day-ring minute
python3 execution/build_geodata.py         # rebuild src/pkjs/geodata.js from Natural Earth
python3 -m pytest tests/ -q                # host tests (C harness + node vs Python projection)
python3 execution/emulator_check.py        # emulator scenarios → .tmp/emulator/sheet.png
python3 execution/build.py --debug         # SA_DEBUG=1 readout
```

Install: `cd watchface && pebble install --emulator emery build/watchface.pbw`
(emulator quirks: `.agent/skills/_PEBBLE/SKILL.md` in the workspace).

## Layout

```text
watchface/src/c/main.c        render, events, lifecycle
watchface/src/c/place.c       day-ring radius, label rule, region cells (pure C, host-tested)
watchface/src/c/geo_store.c   receives the phone's chunks
watchface/src/c/heading.c     flick-started compass sessions, view toggle
watchface/src/c/config.h      SA_DEBUG, SA_TEST_HEADING, SA_TEST_MINUTE defaults
watchface/src/pkjs/geo.js     AEQD projection, cities, region cells, packing (pure, node-tested)
watchface/src/pkjs/index.js   location → chunks to the watch
watchface/src/pkjs/geodata.js GENERATED Natural Earth bundle (public domain)
watchface/src/pkjs/dev.json   emulator location fixture; must ship as {}
execution/                    build, build_geodata, emulator_check
directives/build_watchface.md  goal, architecture, Validation Contract
```

---

# straight-ahead

Scaffolded from `project-template` using the `workflow-python` profile.

## Project Profile

- `profile`: `workflow-python`
- `deploy`: `local-only`
- `runtime`: `python`
- `loop_mode`: `data`
- `knowledge_level`: `heavy`

## Workshop Standard

This project follows the 3-layer architecture:

```text
directives/     Layer 1 — What to do
agent           Layer 2 — Decide → Delegate → Verify
execution/      Layer 3 — Deterministic execution (when this profile uses it)
```

## Next Steps

1. Fill in the actual project description here.
2. Update `task.md` with the first real milestone.
3. Populate `knowledge/context.md`.
4. Draft the first workflow directive in `directives/`.

Map data: [Natural Earth](https://www.naturalearthdata.com/) (public domain).
