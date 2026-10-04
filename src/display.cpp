#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include "display.h"

namespace {

bool ready = false;

const int WEIGHT_BASELINE = 31;  // bottom row; 25 px digits reach up to row 7
const int WEIGHT_RIGHT = 120;    // digits end here, the small "g" follows

}  // namespace

bool SetupDisplay(Adafruit_SSD1306& display) {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return false;
  }
  // begin() doesn't check that anything is on the bus, so ask
  Wire.beginTransmission(SCREEN_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("No OLED at 0x3C; check the screen wiring"));
    return false;
  }
  ready = true;
  display.clearDisplay();
  display.display();
  return true;
}

void DisplayMessage(Adafruit_SSD1306& display, String message, int pause) {
  if (ready) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(&FreeMono9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(true);
    display.setCursor(0, 12);
    display.print(message.c_str());
    display.display();
  }
  if (pause != 0) delay(pause);
}

void DisplayLines(Adafruit_SSD1306& display, const String& text) {
  if (!ready) return;
  display.clearDisplay();
  display.setFont(NULL);  // built-in 6 x 8 font
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setCursor(0, 0);
  display.print(text.c_str());
  display.display();
}

void DisplayWeight(Adafruit_SSD1306& display, float weight, const String& left, const String& right) {
  if (!ready) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  // Status line, built-in font
  display.setFont(NULL);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(left.c_str());
  if (right.length() > 0) {
    display.setCursor(display.width() - 6 * right.length(), 0);
    display.print(right.c_str());
  }

  // Weight, right-aligned so the digits don't shift as it changes
  char text[12];
  if (isnan(weight)) {
    strcpy(text, "--");
  } else {
    if (fabsf(weight) < 0.05f) weight = 0.0f;  // no "-0.0"
    snprintf(text, sizeof text, "%.1f", weight);
  }
  display.setFont(&FreeSansBold18pt7b);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, WEIGHT_BASELINE, &x1, &y1, &w, &h);
  display.setCursor(WEIGHT_RIGHT - x1 - w, WEIGHT_BASELINE);
  display.print(text);

  display.setFont(NULL);
  display.setCursor(WEIGHT_RIGHT + 2, WEIGHT_BASELINE - 7);
  display.print("g");
  display.display();
}
