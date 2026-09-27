# Straight Ahead

A Pebble Time 2 (emery) watch face by Globe & Atlas. The world is drawn in azimuthal-equidistant
projection, centred on you and turned to the way you face. In that projection every great circle
through the centre is a straight line, so the vertical line on the face *is* the great circle you
are facing. Up is where it leads (the label will name its first landfall), down is where it came
from, and the rim is your antipode, 20,015 km away. Rings mark 5,000 km steps.

Status: **M0 scaffold**. Disc, rings, north tick, facing line and flick-started compass sessions
work; coastlines and landfall come next (see `task.md`).

## Use

- North-up by default. **Flick your wrist** to turn the map to your heading for 60 s (a flick
  during a session extends it). The compass only runs during sessions, to save battery.
- Header: time; `N-UP` or `HDG 045` (`?` while calibrating, `NO CMPS` if unavailable);
  `NO FIX` until the phone sends a location.

## Build

```sh
python3 execution/build.py                 # production → dist/straight-ahead.pbw
python3 execution/build.py --heading 45    # fixture: frozen heading (the emery emulator has no compass)
python3 execution/build.py --debug         # SA_DEBUG=1 readout
```

Install: `cd watchface && pebble install --emulator emery build/watchface.pbw`
(emulator quirks: `.agent/skills/_PEBBLE/SKILL.md` in the workspace).

## Layout

```text
watchface/src/c/main.c      render, events, lifecycle
watchface/src/c/heading.c   flick-started compass sessions
watchface/src/c/config.h    SA_DEBUG, SA_TEST_HEADING defaults
watchface/src/pkjs/index.js location → watch (projection, landfall, declination to come)
execution/build.py          production and fixture builds
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
