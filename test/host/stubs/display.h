#pragma once
#include "Arduino.h"
#include "Adafruit_SSD1306.h"
// Records what would be on the screen
extern std::string lastScreen;
inline void DisplayMessage(Adafruit_SSD1306&, String m, int = 0) { lastScreen = m.s; }
inline void DisplayLines(Adafruit_SSD1306&, const String& t) { lastScreen = t.s; }
