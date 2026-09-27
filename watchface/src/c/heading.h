#pragma once
#include <pebble.h>

// The compass is the face's main battery risk, so it only runs in 60 s sessions (heading-up).
// A session starts when the face opens, when the wrist is raised (raise.c, from 10 Hz accel
// batches) or on a flick. A raise during a session extends it; a flick during a session extends
// it and toggles the world view. When it ends the face returns to the day ring, north-up.
#define HEADING_SESSION_MS 60000

// Accelerometer samples per wake-up at 10 Hz: one wake-up a second for raise detection.
#define RAISE_BATCH 10

// Updates arrive only when the heading moves at least this much (5 degrees).
#define HEADING_FILTER (TRIG_MAX_ANGLE / 72)

typedef struct {
  int32_t heading;  // clockwise from magnetic north, TRIG_MAX_ANGLE units; 0 when north-up
  bool live;        // a compass session is running
  bool world;       // world view requested (toggled by flicks during a session)
  CompassStatus status;
} Heading;

typedef void (*HeadingHandler)(const Heading *heading);

void heading_start(HeadingHandler on_change);
void heading_stop(void);
