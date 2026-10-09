#pragma once
#include <HX711_ADC.h>
//HX711 pins:
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 7; //mcu > HX711 sck pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// DEFAULT_CAL_FACTOR (used when no calibration has been saved) is in config.h
#include "config.h"

// Start the HX711, tare, and load the calibration. Returns false if the
// HX711 doesn't answer (message says what to check).
bool SetupLoadCell(HX711_ADC& LoadCell, String& message);
void saveCalFactor(float calFactor);
