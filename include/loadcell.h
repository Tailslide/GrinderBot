#pragma once
#include <HX711_ADC.h>
#include <functional>
//HX711 pins:

//const int HX711_dout = 11; //mcu > HX711 dout pin
//const int HX711_sck = 12; //mcu > HX711 sck pin
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 7; //mcu > HX711 sck pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Calibration factor used when none has been saved to flash.
// Uploading new firmware wipes the saved value (FlashStorage keeps it inside
// the program image), so once you've calibrated, put the factor printed by the
// 'r' command here and readings stay in grams after every upload.
const float DEFAULT_CAL_FACTOR = 1.0f;

// Define the callback type as a typedef for convenience
typedef std::function<void(const char*)> DisplayCallback;

bool SetupLoadCell(HX711_ADC& LoadCell, String& message );
void calibrate(HX711_ADC& LoadCell, DisplayCallback);
void changeSavedCalFactor(HX711_ADC& LoadCell);
void saveCalFactor(float calFactor);
