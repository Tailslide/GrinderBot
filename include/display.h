#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/DejaVu_Sans_Mono_14.h>
#include <Chrono.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x6B ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

void SetupDisplay(Adafruit_SSD1306& display);
void DisplayWeight(Adafruit_SSD1306& display, Chrono fortimer, float weight);
void DisplayMessage(Adafruit_SSD1306& display, String message, int pause=0);
