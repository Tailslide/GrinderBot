#pragma once

// SAMD21 hardware watchdog. If the firmware stops feeding it, the board
// resets, and ServoBegin() parks the arm first thing on boot, which lets go
// of the grinder button.
//
// Two periods:
//  - WATCHDOG_PERIOD_MS while the servo is away from rest (holding the
//    button), so a hang mid-grind stops the grinder within 2 s
//  - WATCHDOG_IDLE_PERIOD_MS the rest of the time (armed at the end of
//    setup()), which only catches real lock-ups: a wedged Wi-Fi module, or a
//    library call waiting forever on an unplugged HX711. Slow Wi-Fi/MQTT
//    calls and the blocking calibration steps stay well inside it.
const unsigned long WATCHDOG_PERIOD_MS = 2000;
const unsigned long WATCHDOG_IDLE_PERIOD_MS = 16000;

void WatchdogArm(unsigned long periodMs);  // start, or change the period (2, 4, 8 or 16 s)
void WatchdogDisarm();
void WatchdogFeed();  // call every loop (and inside long jobs)
bool WatchdogCausedLastReset();
