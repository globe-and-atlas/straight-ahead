#include "raise.h"

void raise_reset(Raise *r) {
  r->armed = false;
}

bool raise_step(Raise *r, int16_t z) {
  if (z >= RAISE_DOWN_Z_MG) {
    r->armed = true;
    return false;
  }
  if (z <= RAISE_UP_Z_MG && r->armed) {
    r->armed = false;
    return true;
  }
  return false;
}
