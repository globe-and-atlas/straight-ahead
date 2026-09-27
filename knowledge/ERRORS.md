# Errors

Record deterministic errors, root causes, and fixes here.

## 2026-09-27 — Unavailable compass shown as HDG 000
- Error: header showed a confident `HDG 000` in the emulator.
- Cause: handler rejected only `CompassStatusDataInvalid`; the emery emulator sends `CompassStatusUnavailable` (-1) with heading 0.
- Fix: accept only `>= CompassStatusCalibrating`; show `NO CMPS` when unavailable.
- Graduated to: `/Users/danielbally/Git/.agent/skills/_PEBBLE/SKILL.md`.

## 2026-09-27 — Emulator showed another project's face after "install succeeded" (infrastructure)
- Stale shared QEMU state; fixed by kill + boot + reinstall. Logged only; see `_PEBBLE` skill.
