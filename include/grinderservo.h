#pragma once

// The servo that presses the grinder's manual button.
//
// Positions are servo pulse widths in microseconds. The defaults are the exact
// pulses Greg's first test produced with attach(servoPin, 0, 45): the Servo
// library reads attach()'s min/max as microseconds (not degrees), and 0/45
// overflowed its internal limits to a 1024-2096 us range, so write(0) sent
// 1024 us and write(45) sent 1292 us. Saved positions (set from the pads)
// replace these. SERVO_DEFAULT_REST_US / SERVO_DEFAULT_PRESS_US are in config.h.
#include "config.h"
const int SERVO_MIN_US = 544;             // limits for adjusting from the pads (the Servo
const int SERVO_MAX_US = 2400;            //   library clamps to these anyway)

// ServoPress() eases onto the button over this long instead of slamming it
const unsigned long SERVO_PRESS_RAMP_MS = 250;
// Once back at rest for this long, the servo stops getting pulses so it
// doesn't hum or buzz while idle (nothing pushes on the arm at rest)
const unsigned long SERVO_DETACH_MS = 800;

// Park the servo at the rest position and start driving it. Call first thing
// in setup(): the rest pulse is set before attach(), so the arm doesn't swing
// to the library's 1500 us default at power-up.
void ServoBegin(int pin);
// Call every loop: runs the press ramp and detaches after resting
void ServoUpdate();

int ServoRestUs();
int ServoPressUs();
void ServoSetPositions(int restUs, int pressUs);  // in memory; SaveSettings() persists

void ServoWriteUs(int us);  // move straight to any pulse width (clamped to the limits)
void ServoRest();           // straight back to rest (fast, so a grind stops on time)
void ServoPress();          // ease onto the button
bool ServoAtRest();         // commanded to rest and already there

// Anywhere other than rest, the watchdog (watchdog.h) runs with its short
// period; back at rest it goes back to the long one.

// Write the settings to flash once the servo is at rest and no longer getting
// pulses. A flash write stalls the CPU, interrupts included, for a few ms; if
// that lands mid-pulse the pulse comes out several ms long, which can kick
// the servo towards the button. Settings changed while idle are saved within
// one loop.
void ServoSaveSettingsAtRest();
