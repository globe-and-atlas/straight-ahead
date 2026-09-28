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

## 2026-09-27 — Verifier findings on the day ring (APPROVE WITH NITS)
- Major: compass dropping to DataInvalid mid-session left the map rotated to the stale heading while label/header used north. Fix: heading.c resets heading to 0 below Calibrating.
- Minor: every 30-min refresh cleared all ready flags (face blanked). Fix: no global reset on LAT; phone skips resend when moved < 1 nmi.
- Minor (suspected): empty payload kinds sent as zero-length DATA could loop retries forever. Fix: TOTAL 0 without DATA, watch accepts it; retries capped at 5. Point Nemo emulator scenario added.
- Open: magnetic vs true north (M4, ~2° at Spring TX, 15–20° in the Pacific NW/Alaska); world-view landfall (M3); no persistence across relaunch without phone.

## 2026-09-27 — Shell environment published in watchface/.lock-waf_darwin_build (security)
- Error: waf's lock file (written by `pebble build`) was committed in every commit since the scaffold and pushed publicly. It contains the full shell environment: POSTHOG_API_KEY (phc_ project key), CLAUDE_CODE_MESSAGING_TOKEN (session-scoped), session ids, paths.
- Cause: this repo's .gitignore (from project-template) had no `.lock-waf*` entry; `git add -A` picked it up.
- Fix: untracked + ignored (64e39b1); project-template .gitignore and the _PEBBLE skill updated. History purge + force-push pending owner decision; see SESSION.md.
