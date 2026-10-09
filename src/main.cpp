/**************************************************************************
 GrinderBot - coffee grinder scale that grinds by weight
 **************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include "loadcell.h"
#include "display.h"
#include "FlashStore.h"
#include "pads.h"
#include "grinderservo.h"
#include "grind.h"
#include "net.h"
#include "watchdog.h"
#include "ui.h"

HX711_ADC LoadCell(HX711_dout, HX711_sck);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Touch pads (TTP223 boards: output is HIGH while the pad is touched)
Pad menuPad(0);  // ≡    tap: tare, hold 2 s: menu
Pad upPad(1);    // 1 ▲  tap: dose 1, hold: edit it; up in menus
Pad downPad(2);  // 2 ▼  tap: dose 2, hold: edit it; down in menus
Pad okPad(3);    // OK   grind / select
const int servoPin = 4;
const int buzzerPin = 5;

const unsigned int BEEP_HZ = 3000;
const unsigned long BEEP_MS = 50;
const unsigned long BEEP_GAP_MS = 120;
const unsigned long DISPLAY_INTERVAL_MS = 150;
const unsigned long NO_DATA_MS = 1000;  // idle: say so if the load cell goes quiet this long

bool scaleOk = false;
float weight = 0.0f;
unsigned long lastData = 0;
unsigned long tDisplay = 0;
int beepsLeft = 0;
unsigned long nextBeep = 0;

// Non-blocking beep. tone() runs on timer TC5; the Servo library uses TC4,
// so the two don't interfere.
void beep() {
  tone(buzzerPin, BEEP_HZ, BEEP_MS);
}

void beepTimes(int n) {
  beepsLeft = n;
  nextBeep = millis();
}

void updateBeeps() {
  if (beepsLeft > 0 && (long)(millis() - nextBeep) >= 0) {
    beep();
    beepsLeft--;
    nextBeep = millis() + BEEP_MS + BEEP_GAP_MS;
  }
}

void setup() {
  // Park the servo first, before anything slow happens
  ServoBegin(servoPin);

  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");
  if (WatchdogCausedLastReset()) Serial.println("Restarted by the watchdog (the firmware had hung)");
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

  if (!SetupDisplay(display)) {
    // Keep going without a screen, but make it obvious something's wrong
    tone(buzzerPin, 800, 600);
    delay(800);
  }
  if (WatchdogCausedLastReset()) DisplayMessage(display, "Watchdog\r\nrestart", 2000);

  String message;
  scaleOk = SetupLoadCell(LoadCell, message);
  if (message != "") DisplayMessage(display, message, scaleOk ? 1500 : 3000);
  GrindBegin(scaleOk);
  NetBegin();
  lastData = millis();
  WatchdogArm(WATCHDOG_IDLE_PERIOD_MS);  // short period whenever the servo leaves rest
  Serial.println("Started");
}

void loop() {
  WatchdogFeed();

  menuPad.update();
  upPad.update();
  downPad.update();
  okPad.update();

  bool newData = false;
  bool tareDone = false;
  if (scaleOk) {
    if (LoadCell.update()) {
      newData = true;
      weight = LoadCell.getData();
      lastData = millis();
    }
    // getTareStatus() clears the flag when read, so read it once here
    tareDone = LoadCell.getTareStatus();
    if (tareDone) Serial.println("Tare complete");
  }

  // Grind logic before the pads, so safety stops never wait on the UI
  if (GrindUpdate(LoadCell, newData, weight, tareDone)) {
    const GrindRecord& r = GrindLastRecord();
    // Two beeps: done. Three: a safety stop. (A pad stop already beeped.)
    if (r.result == GrindResult::Done) beepTimes(2);
    else if (r.result != GrindResult::Stopped) beepTimes(3);
    NetPublishGrind(r);
  }

  UiAction action = UiUpdate(LoadCell, display, menuPad, upPad, downPad, okPad, tareDone);
  ServoUpdate();
  updateBeeps();

  if (action == UiAction::ShowWeight && millis() - tDisplay >= DISPLAY_INTERVAL_MS) {
    // (Subtracting times keeps working when millis() wraps.)
    tDisplay = millis();
    String left = GrindStatus();
    String right = GrindStatusRight();
    if (scaleOk && !GrindBusy() && millis() - lastData > NO_DATA_MS) left = "No load cell data";
    if (right.length() == 0 && NetConnected()) right = "HA";
    DisplayWeight(display, scaleOk ? weight : NAN, left, right);
  }

  bool quiet = !GrindBusy() && ServoAtRest();
  NetUpdate(quiet, quiet && UiIdle());

  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    // tare, only from the weight screen (not mid-calibration or servo setup)
    if (inByte == 't' && !GrindBusy() && UiIdle() && ServoAtRest()) GrindZero(LoadCell);
  }

  delay(5);
}
