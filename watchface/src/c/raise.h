#pragma once
#include <stdbool.h>
#include <stdint.h>

// Wrist-raise detector (pure, host-tested). A raise is the watch going from face-vertical (arm
// hanging, screen facing sideways) to face-up (screen toward the sky, i.e. toward the wearer).
// Pebble z is -1000 mG face-up flat and ~0 with the screen vertical.
#define RAISE_DOWN_Z_MG (-250)  // z at or above this: arm down / screen vertical
#define RAISE_UP_Z_MG (-600)    // z at or below this: screen facing up

typedef struct {
  bool armed;  // seen "down" since the last raise
} Raise;

void raise_reset(Raise *r);

// Feed one sample's z (mG). Returns true exactly once per down-to-up transition.
bool raise_step(Raise *r, int16_t z);
