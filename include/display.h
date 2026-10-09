#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// Start the OLED. Returns false if it isn't there (nothing answers at
// SCREEN_ADDRESS) or its buffer can't be allocated; the scale then keeps
// running without a screen and every Display* call does nothing.
bool SetupDisplay(Adafruit_SSD1306& display);

// Weight screen: a small status line on top (left and right text, ~21
// characters together) and the weight in large digits underneath.
// A NaN weight shows as "--".
void DisplayWeight(Adafruit_SSD1306& display, float weight, const String& left, const String& right);

// Two lines in the 9 pt font (~11 characters each, "\r\n" between them)
void DisplayMessage(Adafruit_SSD1306& display, String message, int pause=0);

// Up to four lines in the small built-in font (21 characters each, "\n" between)
void DisplayLines(Adafruit_SSD1306& display, const String& text);
