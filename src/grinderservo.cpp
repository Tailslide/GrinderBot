/**
 grinderservo.cpp - the servo that presses the grinder's manual button
**/

#include <Arduino.h>
#include <Servo.h>
#include "grinderservo.h"
#include "FlashStore.h"

namespace {

Servo servo;
int restUs = SERVO_DEFAULT_REST_US;
int pressUs = SERVO_DEFAULT_PRESS_US;

int clampUs(int us) {
  if (us < SERVO_MIN_US) return SERVO_MIN_US;
  if (us > SERVO_MAX_US) return SERVO_MAX_US;
  return us;
}

}  // namespace

void ServoBegin(int pin) {
  // Use saved positions if there are any (0 means never saved)
  if (settings.valid && settings.servoRestUs != 0 && settings.servoPressUs != 0) {
    restUs = clampUs(settings.servoRestUs);
    pressUs = clampUs(settings.servoPressUs);
  }
  servo.writeMicroseconds(restUs);  // before attach(): the first pulse is already "rest"
  servo.attach(pin);
}

int ServoRestUs() { return restUs; }
int ServoPressUs() { return pressUs; }

void ServoSetPositions(int newRestUs, int newPressUs) {
  restUs = clampUs(newRestUs);
  pressUs = clampUs(newPressUs);
  settings.servoRestUs = restUs;
  settings.servoPressUs = pressUs;
}

void ServoWriteUs(int us) { servo.writeMicroseconds(clampUs(us)); }
void ServoRest() { servo.writeMicroseconds(restUs); }
void ServoPress() { servo.writeMicroseconds(pressUs); }
