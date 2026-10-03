/**************************************************************************
 GrinderBot - coffee grinder scale
 **************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <Chrono.h>
#include <Servo.h>
#include "loadcell.h"
#include "display.h"
#include "FlashStore.h"

HX711_ADC LoadCell(HX711_dout, HX711_sck);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Chrono timer(Chrono::SECONDS);
Servo grinderServo;

// Touch pads (TTP223 boards: output is HIGH while the pad is touched)
const int menuPin = 0;  // ≡
const int upPin = 1;    // 1 ▲
const int downPin = 2;  // 2 ▼
const int okPin = 3;    // OK
const int servoPin = 4;
const int buzzerPin = 5;

// Servo pulse widths in microseconds.
// These are the exact pulses the earlier attach(servoPin, 0, 45) produced for
// write(0) and write(45). The Servo library reads attach()'s min/max as pulse
// widths in microseconds (not degrees), and 0/45 overflowed its internal
// limits to a 1024-2096 us range. Keeping the same pulses keeps the arm moving
// exactly as it did in the test.
const int SERVO_REST_US = 1024;   // arm clear of the manual button
const int SERVO_PRESS_US = 1292;  // arm holding the manual button down
const int SERVO_STEP_US = 6;      // ~1 degree per step, like the original sweep

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
// code will replace it.
void servoPressTest() {
  for (int us = SERVO_REST_US; us <= SERVO_PRESS_US; us += SERVO_STEP_US) {
    grinderServo.writeMicroseconds(us);
    delay(15);
  }
  grinderServo.writeMicroseconds(SERVO_PRESS_US);
  delay(5000);
  grinderServo.writeMicroseconds(SERVO_REST_US);
}

void setup() {
  // Park the servo first. Setting the pulse before attach() means the very
  // first pulse is the rest position; attach() on its own starts at 1500 us,
  // which is past the press position.
  grinderServo.writeMicroseconds(SERVO_REST_US);
  grinderServo.attach(servoPin);

  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");
  pinMode(menuPin, INPUT);
  pinMode(upPin, INPUT);
  pinMode(downPin, INPUT);
  pinMode(okPin, INPUT);
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
bool menuWasDown = false;

void loop() {
  static bool newDataReady = false;

  // Act once per touch of the menu pad (on the press, not while held)
  bool menuDown = (digitalRead(menuPin) == HIGH);
  if (menuDown && !menuWasDown) {
    Serial.println("Menu pad touched");
    beep();
    servoPressTest();
  }
  menuWasDown = menuDown;

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset. Subtracting times (rather than
  // comparing millis() > t + interval) keeps working when millis() wraps.
  if (newDataReady && (millis() - tDisplay >= DISPLAY_INTERVAL_MS)) {
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

  // check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

  delay(10);
}
