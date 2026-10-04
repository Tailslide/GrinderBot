#pragma once

// SAMD21 hardware watchdog, used only while the servo is away from rest
// (holding the grinder's button). If the firmware hangs during a grind, the
// watchdog resets the board within WATCHDOG_PERIOD_MS; ServoBegin() then parks
// the arm first thing, which lets go of the button and stops the grinder.
//
// It's off the rest of the time, so slow Wi-Fi/MQTT calls and the blocking
// calibration steps can't trip it (they only run with the arm at rest).
const unsigned long WATCHDOG_PERIOD_MS = 2000;  // 2048 cycles of the ~1 kHz watchdog clock

void WatchdogArm();     // start (no-op if already running)
void WatchdogDisarm();  // stop (no-op if not running)
void WatchdogFeed();    // call every loop while armed; cheap when disarmed
bool WatchdogCausedLastReset();
