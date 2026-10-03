/**************************************************************************
 GrinderBot - coffee grinder scale
 **************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <Chrono.h>
#include "loadcell.h"
#include "display.h"
#include "FlashStore.h"
#include "pads.h"
#include "grinderservo.h"
#include "ui.h"

HX711_ADC LoadCell(HX711_dout, HX711_sck);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Chrono timer(Chrono::SECONDS);

// Touch pads (TTP223 boards: output is HIGH while the pad is touched)
Pad menuPad(0);  // ≡   tap: servo test, hold 2 s: menu (Calibrate / Servo pos)
Pad upPad(1);    // 1 ▲
Pad downPad(2);  // 2 ▼
Pad okPad(3);    // OK
const int servoPin = 4;
const int buzzerPin = 5;

const int SERVO_TEST_STEP_US = 6;  // ~1 degree per step, like the original sweep
const unsigned int BEEP_HZ = 3000;
const unsigned long BEEP_MS = 50;
const unsigned long DISPLAY_INTERVAL_MS = 150;

// Show calibration prompts on the screen
void ScreenDisplay(const char* message) {
  DisplayMessage(display, message);
}

// Non-blocking beep. tone() runs on timer TC5; the Servo library uses TC4,
// so the two don't interfere.
void beep() {
  tone(buzzerPin, BEEP_HZ, BEEP_MS);
}

// Temporary test from commit 1ec3ed7: slowly press the manual button, hold it
// for 5 s, then release. It blocks the loop while running; the grind-by-weight
// code will replace it. Uses the saved rest/press positions.
void servoPressTest() {
  int from = ServoRestUs();
  int to = ServoPressUs();
  int step = (to > from) ? SERVO_TEST_STEP_US : -SERVO_TEST_STEP_US;
  for (int us = from; (step > 0) ? (us < to) : (us > to); us += step) {
    ServoWriteUs(us);
    delay(15);
  }
  ServoPress();
  delay(5000);
  ServoRest();
}

void setup() {
  // Park the servo first, before anything slow happens
  ServoBegin(servoPin);

  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");
  Serial.print("Servo rest ");
  Serial.print(ServoRestUs());
  Serial.print(" us, press ");
  Serial.print(ServoPressUs());
  Serial.println(" us");
  menuPad.begin();
  upPad.begin();
  downPad.begin();
  okPad.begin();
  pinMode(buzzerPin, OUTPUT);
  SetupDisplay(display);
  String message;
  SetupLoadCell(LoadCell, message);
  if (message != "") DisplayMessage(display, message, 3000);
  timer.restart();
  DisplayWeight(display, timer, 0.0f);
  display.display();
  Serial.println("Started");
}

unsigned long tDisplay = 0;

void loop() {
  static bool newDataReady = false;

  menuPad.update();
  upPad.update();
  downPad.update();
  okPad.update();

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // check if the last tare operation is complete. getTareStatus() clears the
  // flag when read, so read it once here and hand it to whoever needs it.
  bool tareDone = LoadCell.getTareStatus();
  if (tareDone) Serial.println("Tare complete");

  UiAction action = UiUpdate(LoadCell, display, menuPad, upPad, downPad, okPad, tareDone);
  if (action == UiAction::ServoTest) {
    Serial.println("Menu pad tapped");
    beep();
    servoPressTest();
  } else if (action == UiAction::ShowWeight && newDataReady &&
             (millis() - tDisplay >= DISPLAY_INTERVAL_MS)) {
    // get smoothed value from the dataset. Subtracting times (rather than
    // comparing millis() > t + interval) keeps working when millis() wraps.
    float weight = LoadCell.getData();
    newDataReady = false;
    tDisplay = millis();
    DisplayWeight(display, timer, weight);
    display.display();
  }

  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();                   // tare
    else if (inByte == 'r') calibrate(LoadCell, ScreenDisplay);  // calibrate with a known mass
    else if (inByte == 'c') changeSavedCalFactor(LoadCell);      // type in a calibration factor
  }

  delay(10);
}
