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

## 2026-09-27 — Day ring as the default view (option C)
- Decision: default view is a 24° AEQD disc with a ring at minutes-since-midnight nmi and a named place ahead; the whole-world view is a second flick away.
- Alternatives: world view only (novel but less glanceable); day ring only (loses the antipode/great-circle view).
- Reason: the ring reads as a clock at a glance; the world view keeps the novelty. Distance rule is shared with From Here (noted in README).

## 2026-09-27 — Label rule and data
- City window ±25 nmi / ±12°, scored by radial + lateral miss; fallback 36×24 region cells (sea → country → US state painted on a 0.5° grid). Cities sorted by population so crowded regions drop small cities, never far ones. Seas from NE 1:50m (1:110m lacks the Gulf of California); English names preferred.
- World coastline: project full 1:110m then thin (simplifying in degree space first dropped Asia near the rim).

## 2026-09-27 — Sessions start on wrist raise and on open (middle ground)
- Decision: 60 s heading-up session on face open, on wrist raise (z down→up via 10 Hz accel, 1 wake/s), or flick; flick in-session toggles world view after a 2 s grace.
- Alternatives: flick only (not discoverable — user saw a static map); always-on compass (battery unknown).
- Reason: user chose the middle ground; battery cost of 10 Hz accel + sessions to be measured in the M1 on-watch trial.
