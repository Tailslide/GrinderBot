#pragma once
#include <HX711_ADC.h>
//HX711 pins:
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 7; //mcu > HX711 sck pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Calibration factor used when none has been saved to flash.
// Uploading new firmware wipes the saved value (FlashStorage keeps it inside
// the program image), so once you've calibrated from the panel, put the factor
// it prints over serial here and readings stay in grams after every upload.
const float DEFAULT_CAL_FACTOR = 1.0f;

// Start the HX711, tare, and load the calibration. Returns false if the
// HX711 doesn't answer (message says what to check).
bool SetupLoadCell(HX711_ADC& LoadCell, String& message);
void saveCalFactor(float calFactor);
