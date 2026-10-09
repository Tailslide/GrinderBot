#pragma once
#include <HX711_ADC.h>
#include <Adafruit_SSD1306.h>

// Calibrate the scale from the touch pads and screen, no computer needed.
//   Hold ≡       menu, then Calibrate
//   OK           tare the empty scale
//   ▲ / ▼        reference weight, 100-1000 g in 100 g steps
//   OK           measure, then show the new reading
//   OK           save (≡ cancels at any step and keeps the old calibration)
bool PanelCalActive();
void PanelCalStart(HX711_ADC& LoadCell, Adafruit_SSD1306& display);
void PanelCalUpdate(HX711_ADC& LoadCell, Adafruit_SSD1306& display,
                    bool cancel, bool up, bool down, bool ok, bool tareDone);
