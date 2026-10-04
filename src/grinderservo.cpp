/**
 grinderservo.cpp - the servo that presses the grinder's manual button
**/

#include <Arduino.h>
#include <Servo.h>
#include "grinderservo.h"
#include "watchdog.h"
#include "FlashStore.h"

namespace {

Servo servo;
int servoPin = -1;
int restUs = SERVO_DEFAULT_REST_US;
int pressUs = SERVO_DEFAULT_PRESS_US;
bool attached = false;

int currentUs = SERVO_DEFAULT_REST_US;  // last pulse sent
int fromUs = SERVO_DEFAULT_REST_US;     // ramp start
int targetUs = SERVO_DEFAULT_REST_US;   // where the arm is headed
unsigned long rampStart = 0;
unsigned long rampMs = 0;
unsigned long restSince = 0;
bool savePending = false;

int clampUs(int us) {
  if (us < SERVO_MIN_US) return SERVO_MIN_US;
  if (us > SERVO_MAX_US) return SERVO_MAX_US;
  return us;
}

void output(int us) {
  currentUs = us;
  servo.writeMicroseconds(us);  // set before attach(), so the first pulse is right
  if (!attached) {
    servo.attach(servoPin);
    attached = true;
  }
}

void moveTo(int us, unsigned long ms) {
  us = clampUs(us);
  fromUs = currentUs;
  targetUs = us;
  rampStart = millis();
  rampMs = ms;
  // Pulse first: letting go of the button shouldn't wait on the watchdog's
  // few-ms register sync
  output(ms == 0 ? us : fromUs);
  if (us == restUs) {
    restSince = millis();
    WatchdogArm(WATCHDOG_IDLE_PERIOD_MS);
  } else {
    WatchdogArm(WATCHDOG_PERIOD_MS);
  }
}

}  // namespace

void ServoBegin(int pin) {
  servoPin = pin;
  // Use saved positions if there are any (0 means never saved)
  if (settings.valid && settings.servoRestUs != 0 && settings.servoPressUs != 0) {
    restUs = clampUs(settings.servoRestUs);
    pressUs = clampUs(settings.servoPressUs);
  }
  currentUs = fromUs = targetUs = restUs;
  rampMs = 0;
  restSince = millis();
  output(restUs);
}

void ServoUpdate() {
  unsigned long now = millis();
  if (currentUs != targetUs) {
    unsigned long t = now - rampStart;
    if (rampMs == 0 || t >= rampMs) {
      output(targetUs);
    } else {
      output(fromUs + (long)(targetUs - fromUs) * (long)t / (long)rampMs);
    }
  }
  if (attached && ServoAtRest() && now - restSince >= SERVO_DETACH_MS) {
    servo.detach();
    // detach() can land in the middle of a pulse, and the library only ends
    // pulses for attached servos, so make sure the line is left low
    digitalWrite(servoPin, LOW);
    attached = false;
  }
  if (savePending && !attached) {
    savePending = false;
    SaveSettings();
  }
}

int ServoRestUs() { return restUs; }
int ServoPressUs() { return pressUs; }

void ServoSetPositions(int newRestUs, int newPressUs) {
  restUs = clampUs(newRestUs);
  pressUs = clampUs(newPressUs);
  settings.servoRestUs = restUs;
  settings.servoPressUs = pressUs;
}

void ServoWriteUs(int us) { moveTo(us, 0); }
void ServoRest() { moveTo(restUs, 0); }
void ServoPress() { moveTo(pressUs, SERVO_PRESS_RAMP_MS); }
bool ServoAtRest() { return targetUs == restUs && currentUs == restUs; }
void ServoSaveSettingsAtRest() { savePending = true; }
