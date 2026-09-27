#pragma once

// Compile-time switches. Defaults live here because CloudPebble ignores the wscript.

// 1 = draw the raw heading, compass status and location in the lower left.
#ifndef SA_DEBUG
#define SA_DEBUG 0
#endif

// 0..359 freezes the heading (emulator fixture: the emery emulator has no compass). -1 = compass.
#ifndef SA_TEST_HEADING
#define SA_TEST_HEADING -1
#endif

// 0..1439 freezes minutes since midnight for the day ring (emulator fixture). -1 = the clock.
#ifndef SA_TEST_MINUTE
#define SA_TEST_MINUTE -1
#endif
