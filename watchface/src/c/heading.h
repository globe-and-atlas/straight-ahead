#pragma once
#include <pebble.h>

// The compass is the face's main battery risk, so it only runs in short sessions: a wrist flick
// starts one, a flick during a session extends it, and when it ends the map goes north-up.
#define HEADING_SESSION_MS 60000

// Updates arrive only when the heading moves at least this much (5 degrees).
#define HEADING_FILTER (TRIG_MAX_ANGLE / 72)

typedef struct {
  int32_t heading;  // clockwise from magnetic north, TRIG_MAX_ANGLE units; 0 when north-up
  bool live;        // a compass session is running
  CompassStatus status;
} Heading;

typedef void (*HeadingHandler)(const Heading *heading);

void heading_start(HeadingHandler on_change);
void heading_stop(void);
