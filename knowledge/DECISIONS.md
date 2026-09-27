# Decisions

## Initial Decision

- Template profile: `workflow-python`
- Deploy target: `local-only`
- Runtime: `python`

## 2026-09-27 — Phone projects, watch rotates
- Decision: PebbleKit JS does the azimuthal-equidistant projection (float trig, a few times a day); the watch only rotates received points with `sin_lookup`.
- Alternatives: projecting on the watch (heavy trig per point, per relocation); baked images per location (impossible: centre is the wearer).
- Reason: the projection changes only when the wearer moves far; heading changes constantly and rotation is cheap.

## 2026-09-27 — Compass in flick-started sessions
- Decision: compass subscribed only for 60 s after a wrist flick; north-up otherwise.
- Alternatives: always-on compass (battery risk flagged by prior-art research); button-driven (faces get no buttons).
- Reason: battery; the face is legible north-up, heading-up is the reward for a deliberate gesture. M1 measures the real cost.
