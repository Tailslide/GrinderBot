#pragma once
#include <HX711_ADC.h>
#include <Adafruit_SSD1306.h>
#include "pads.h"
#include "beep.h"

// What main.cpp should do after UiUpdate()
enum class UiAction { None, ShowWeight };

// Routes the touch pads, once per loop.
// Weight screen:
//   OK         grind the selected dose (tares first, so put the cup on)
//   tap 1 / 2  select dose 1 / dose 2
//   hold 1 / 2 (2 s) edit that dose
//   tap ≡      tare
//   hold ≡     menu: Calibrate, Servo pos, Dose 1, Dose 2, Offset, Network
//              (▲/▼ choose, OK select, ≡ back)
// While grinding, touching any pad stops the grind.
// UiAction::ShowWeight means the weight screen is showing and may be redrawn.
UiAction UiUpdate(HX711_ADC& LoadCell, Adafruit_SSD1306& display,
                  Pad& menuPad, Pad& upPad, Pad& downPad, Pad& okPad, bool tareDone);

// True when no menu, setting or info screen is open
bool UiIdle();
