---
generated_by: "Claude Code CLI (claude-opus-5-5)"
timestamp: "2026-09-27T10:58:00-05:00"
---

# Build and emulate

1. `python3 execution/build.py` → `dist/straight-ahead.pbw` (production; runs `pebble clean` first).
2. Fixture: `python3 execution/build.py --heading 45` (watchface/build only; never distribute).
3. `cd watchface && pebble install --emulator emery build/watchface.pbw`. If the screen shows
   another face: `pebble kill; pkill -f pypkjs; pkill -f qemu-pebble`, install once to boot,
   wait ~20 s, install again.
4. `pebble screenshot --emulator emery --no-open --no-correction ../.tmp/emulator/x.png`
5. Flick: `pebble emu-tap --emulator emery --direction x+`. The emery emulator has **no compass**
   (status Unavailable → header `NO CMPS`); use the heading fixture for rotation.
6. Verified 2026-09-27: north-up, location fix from pypkjs, fixture 45 puts the north tick at -45°.

## Appstore (repebble)
1. `python3 execution/emulator_check.py --store` → `.tmp/store/emery_*.png` (Greenwich; filenames must start with the platform).
2. `cd watchface && pebble clean && pebble publish --non-interactive --no-gif-all-platforms --name ... --version X --description ... --release-notes ... --screenshots ../.tmp/store/emery_*.png`
3. Without `--is-published` the release is a draft; publish from https://appstore-api.repebble.com/dashboard. `dev.json` must be `{}` (test_v14 checks).
