#pragma once

// The servo that presses the grinder's manual button.
//
// Positions are servo pulse widths in microseconds. The defaults are the exact
// pulses Greg's first test produced with attach(servoPin, 0, 45): the Servo
// library reads attach()'s min/max as microseconds (not degrees), and 0/45
// overflowed its internal limits to a 1024-2096 us range, so write(0) sent
// 1024 us and write(45) sent 1292 us. Saved positions (set from the pads)
// replace these.
const int SERVO_DEFAULT_REST_US = 1024;   // arm clear of the manual button
const int SERVO_DEFAULT_PRESS_US = 1292;  // arm holding the manual button down
const int SERVO_MIN_US = 500;             // limits for adjusting from the pads
const int SERVO_MAX_US = 2500;

// Park the servo at the rest position and start driving it. Call first thing
// in setup(): the rest pulse is set before attach(), so the arm doesn't swing
// to the library's 1500 us default at power-up.
void ServoBegin(int pin);

int ServoRestUs();
int ServoPressUs();
void ServoSetPositions(int restUs, int pressUs);  // in memory; SaveSettings() persists

void ServoWriteUs(int us);  // move to any pulse width (clamped to the limits)
void ServoRest();
void ServoPress();
