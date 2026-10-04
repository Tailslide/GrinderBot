#pragma once
#include <Adafruit_SSD1306.h>

// Edit one number from the pads, in tenths (e.g. a dose of 18.0 g is 180).
//   ▲ / ▼   change by one step (hold to repeat)
//   OK      save (calls onSave) and go back
//   ≡       cancel
// 20 s without a touch cancels.
// Screen: "Dose 1 18.0" / "^v adj, OK" (title up to 6 characters).
typedef void (*ValueSaveFn)(int tenths);

bool ValueEditActive();
void ValueEditStart(Adafruit_SSD1306& display, const char* title, int tenths, int minTenths,
                    int maxTenths, ValueSaveFn onSave);
void ValueEditUpdate(Adafruit_SSD1306& display, bool cancel, bool up, bool down, bool ok);
