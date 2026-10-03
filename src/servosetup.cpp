/**
 servosetup.cpp - set the servo's rest and press positions from the touch pads
**/

#include <Arduino.h>
#include "servosetup.h"
#include "grinderservo.h"
#include "display.h"
#include "FlashStore.h"
#include "ui.h"

namespace {

enum class Step { Idle, Rest, Press };

const int STEP_US = 10;
const unsigned long IDLE_TIMEOUT_MS = 20000;

Step step = Step::Idle;
int restUs = SERVO_DEFAULT_REST_US;
int pressUs = SERVO_DEFAULT_PRESS_US;
unsigned long lastActivity = 0;

void show(Adafruit_SSD1306& display) {
  String text = (step == Step::Rest) ? "Rest  " : "Press ";
  text = text + String((step == Step::Rest) ? restUs : pressUs) + "\r\n^v adj, OK";
  DisplayMessage(display, text);
}

}  // namespace

bool ServoSetupActive() {
  return step != Step::Idle;
}

void ServoSetupStart(Adafruit_SSD1306& display) {
  restUs = ServoRestUs();
  pressUs = ServoPressUs();
  step = Step::Rest;
  lastActivity = millis();
  ServoWriteUs(restUs);
  Serial.println("Servo setup started");
  show(display);
}

void ServoSetupUpdate(Adafruit_SSD1306& display, bool cancel, bool up, bool down, bool ok) {
  if (step == Step::Idle) return;
  if (cancel || up || down || ok) lastActivity = millis();
  bool timedOut = (millis() - lastActivity >= IDLE_TIMEOUT_MS);

  if (cancel || timedOut) {
    // Nothing was stored while adjusting, so the saved positions are untouched
    step = Step::Idle;
    ServoRest();
    Serial.println(timedOut ? "Servo setup timed out" : "Servo setup cancelled");
    beep();
    DisplayMessage(display, timedOut ? "Timed out" : "Cancelled", 1000);
    return;
  }

  int& value = (step == Step::Rest) ? restUs : pressUs;
  if (up || down) {
    value += up ? STEP_US : -STEP_US;
    if (value < SERVO_MIN_US) value = SERVO_MIN_US;
    if (value > SERVO_MAX_US) value = SERVO_MAX_US;
    ServoWriteUs(value);
    show(display);
  } else if (ok) {
    beep();
    if (step == Step::Rest) {
      step = Step::Press;
      ServoWriteUs(pressUs);
      show(display);
    } else {
      ServoSetPositions(restUs, pressUs);
      SaveSettings();
      ServoRest();
      Serial.print("Servo positions saved: rest ");
      Serial.print(restUs);
      Serial.print(" us, press ");
      Serial.print(pressUs);
      Serial.println(" us");
      step = Step::Idle;
      DisplayMessage(display, "Saved", 1000);
    }
  }
}
