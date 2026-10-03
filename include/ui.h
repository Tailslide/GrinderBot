#pragma once
#include <HX711_ADC.h>
#include <Adafruit_SSD1306.h>
#include "pads.h"

// What main.cpp should do after UiUpdate()
enum class UiAction { None, ServoTest, ShowWeight };

// Routes the touch pads, once per loop:
//   hold ≡   menu: Calibrate / Servo pos (▲/▼ choose, OK select, ≡ back)
//   tap ≡    servo press test (returned as UiAction::ServoTest)
// Otherwise the weight screen is free to update (UiAction::ShowWeight).
UiAction UiUpdate(HX711_ADC& LoadCell, Adafruit_SSD1306& display,
                  Pad& menuPad, Pad& upPad, Pad& downPad, Pad& okPad, bool tareDone);

// Short confirmation beep (defined in main.cpp)
void beep();
