# Errors

Record deterministic errors, root causes, and fixes here.

## 2026-09-27 — Unavailable compass shown as HDG 000
- Error: header showed a confident `HDG 000` in the emulator.
- Cause: handler rejected only `CompassStatusDataInvalid`; the emery emulator sends `CompassStatusUnavailable` (-1) with heading 0.
- Fix: accept only `>= CompassStatusCalibrating`; show `NO CMPS` when unavailable.
- Graduated to: `/Users/danielbally/Git/.agent/skills/_PEBBLE/SKILL.md`.

## 2026-09-27 — Emulator showed another project's face after "install succeeded" (infrastructure)
- Stale shared QEMU state; fixed by kill + boot + reinstall. Logged only; see `_PEBBLE` skill.

## 2026-09-27 — World view missing Asia/Australia
- Cause: coastline simplified in lon/lat before projection; near the antipode consecutive sparse points jumped > 40 units and were dropped as wrap-arounds.
- Fix: project the full coastline, then thin to ≤ 1,500 points. Graduated to: knowledge/domain/projection.md.

## 2026-09-27 — HDG 019 for a 20° heading
- Cause: truncating TRIG units → tenths of a degree twice. Fix: round in `heading10()`.
