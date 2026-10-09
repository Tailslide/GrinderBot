/**
 valueedit.cpp - edit a number (dose, offset) from the touch pads
**/

#include <Arduino.h>
#include "valueedit.h"
#include "display.h"
#include "beep.h"

namespace {

const unsigned long IDLE_TIMEOUT_MS = 20000;

bool active = false;
const char* title = "";
int value = 0;
int minValue = 0;
int maxValue = 0;
ValueSaveFn saveFn = nullptr;
bool wholeUnits = false;
unsigned long lastActivity = 0;

void show(Adafruit_SSD1306& display) {
  char line[40];
  if (wholeUnits) {
    snprintf(line, sizeof line, "%-6s %4d\r\n^v adj, OK", title, value);
  } else {
    snprintf(line, sizeof line, "%-6s %2d.%d\r\n^v adj, OK", title, value / 10, value % 10);
  }
  DisplayMessage(display, String(line));
}

}  // namespace

bool ValueEditActive() { return active; }

void ValueEditStart(Adafruit_SSD1306& display, const char* newTitle, int tenths, int minTenths,
                    int maxTenths, ValueSaveFn onSave, bool whole) {
  title = newTitle;
  wholeUnits = whole;
  minValue = minTenths;
  maxValue = maxTenths;
  value = tenths < minTenths ? minTenths : (tenths > maxTenths ? maxTenths : tenths);
  saveFn = onSave;
  active = true;
  lastActivity = millis();
  show(display);
}

void ValueEditUpdate(Adafruit_SSD1306& display, bool cancel, bool up, bool down, bool ok) {
  if (!active) return;
  if (cancel || up || down || ok) lastActivity = millis();
  bool timedOut = (millis() - lastActivity >= IDLE_TIMEOUT_MS);
  if (cancel || timedOut) {
    active = false;
    beep();
    DisplayMessage(display, timedOut ? "Timed out" : "Cancelled", 1000);
    return;
  }
  if (up && value < maxValue) {
    value++;
    show(display);
  } else if (down && value > minValue) {
    value--;
    show(display);
  } else if (ok) {
    active = false;
    if (saveFn) saveFn(value);
    beep();
    DisplayMessage(display, "Saved", 1000);
  }
}
