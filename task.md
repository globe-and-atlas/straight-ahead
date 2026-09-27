# Task: straight-ahead

## Objective

Ship "Straight Ahead" for Pebble Time 2: the world in azimuthal-equidistant projection, centred on
the wearer and turned to their heading, with the great circle they face labelled by its first
landfall. Directive: `directives/build_watchface.md`.

## Milestones

- [x] M0 — Scaffold: repo, watchface builds, disc + range rings + north tick + facing line, flick-started compass sessions, phone sends location
- [ ] M1 — Compass trial on a real Pebble Time 2: heading accuracy after calibration (V7)
- [x] Sessions on wrist raise / face open (R1–R7)
- [ ] M1 — Compass trial on a real Pebble Time 2: battery cost of raise-triggered sessions + 10 Hz accel over 3 days (V8)
- [x] M2 — `execution/build_geodata.py`: Natural Earth coastline, cities, region grid bundled into `src/pkjs/geodata.js`
- [x] M2 — Phone projects coastline to AEQD around the fix and sends int8 points in AppMessage chunks
- [x] M2 — Watch stores the points and rotates them by heading
- [x] M2 — Host tests (tests/test_geo.py, tests/test_place.py: 35 pass)
- [x] Day ring (option C): ring at minutes-since-midnight nmi, label = city in window → sea/state/country; world view on second flick
- [x] `execution/emulator_check.py` (7 scenarios, D12/D13 pixel checks)
- [x] Creator-verifier pass on the day ring (APPROVE WITH NITS; major + 2 minor fixed)
- [ ] Persist map/cities (`persist_write`) so a relaunch without the phone still shows the map
- [ ] M3 — World view: first landfall along the facing great circle (label row currently `WORLD`)
- [ ] M4 — WMM2025 declination on the phone; watch rotates by true heading
- [ ] M5 — `execution/emulator_check.py`, CloudPebble simulation, README screenshots, own public repo under globe-and-atlas

## Open questions

- Does the compass drain enough to make sessions shorter than 60 s? (M1 answers.)
- World-view landfall label: first land reached after leaving the wearer's landmass, or first country crossed? Decide before M3.
- Day-ring label rule is a default (city in ±25 nmi / ±12°, else sea/state/country); revisit after wearing it.
