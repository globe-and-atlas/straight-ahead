# Task: straight-ahead

## Objective

Ship "Straight Ahead" for Pebble Time 2: the world in azimuthal-equidistant projection, centred on
the wearer and turned to their heading, with the great circle they face labelled by its first
landfall. Directive: `directives/build_watchface.md`.

## Milestones

- [x] M0 — Scaffold: repo, watchface builds, disc + range rings + north tick + facing line, flick-started compass sessions, phone sends location
- [ ] M1 — Compass trial on a real Pebble Time 2: heading accuracy after calibration (V7)
- [ ] M1 — Compass trial on a real Pebble Time 2: battery cost of 20 sessions/day over 3 days (V8)
- [ ] M2 — `execution/build_coastline.py`: Natural Earth 1:110m coastline, simplified, bundled into `src/pkjs`
- [ ] M2 — Phone projects coastline to AEQD around the fix and sends int8 points in AppMessage chunks
- [ ] M2 — Watch stores the points and rotates them by heading
- [ ] M2 — Host tests for V9–V11
- [ ] M3 — Phone computes first landfall every 5° of bearing; watch shows `AHEAD <place> <km>`
- [ ] M4 — WMM2025 declination on the phone; watch rotates by true heading
- [ ] M5 — `execution/emulator_check.py`, CloudPebble simulation, README screenshots, own public repo under globe-and-atlas

## Open questions

- Does the compass drain enough to make sessions shorter than 60 s? (M1 answers.)
- Landfall label: the first land the great circle *reaches* after leaving the wearer's own landmass, or the first *country* crossed? Decide before M3.
