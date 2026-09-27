---
generated_by: "Claude Code CLI (claude-opus-5-5)"
timestamp: "2026-09-27T10:58:00-05:00"
---

# Azimuthal-equidistant projection (as used here)

- Centre = wearer. A point at great-circle distance `d` and initial bearing `b` from the wearer
  plots at radius `d / 20,015 km × DISC_R` and angle `b` (clockwise from up = north).
- Consequences the face relies on: every great circle through the centre is a straight line;
  the rim (d = πR) is a single point, the antipode, smeared into a circle; distortion grows
  toward the rim, so the far hemisphere is unrecognisable. That is expected, not a bug.
- Heading-up: rotate every plotted point by `-heading` (`main.c: polar()`); the facing great
  circle is then the vertical line.
- Distance/bearing formulas for the phone (M2): spherical `d = R·acos(sinφ1 sinφ2 + cosφ1 cosφ2 cosΔλ)`
  (use haversine near 0), `b = atan2(sinΔλ cosφ2, cosφ1 sinφ2 − sinφ1 cosφ2 cosΔλ)`.
- Compass gives magnetic heading only (SDK); true heading needs declination (WMM, M4).
- Prior art: Projections (2017) has a static pole-centred AEQD image; North Analog (2014)
  rotates a dial by compass; Yearlight is orthographic, north-up, no compass.
