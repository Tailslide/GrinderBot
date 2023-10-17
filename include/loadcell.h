#pragma once
#include <HX711_ADC.h>

//HX711 pins:

//const int HX711_dout = 11; //mcu > HX711 dout pin
//const int HX711_sck = 12; //mcu > HX711 sck pin
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 7; //mcu > HX711 sck pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


void SetupLoadCell(HX711_ADC& LoadCell);
void calibrate(HX711_ADC& LoadCell);
void changeSavedCalFactor(HX711_ADC& LoadCell);
