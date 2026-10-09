#pragma once
#include <Adafruit_SSD1306.h>

// Set the servo's rest and press positions from the touch pads.
//   ▲ / ▼   move the arm 10 us per step (hold to repeat); it moves as you adjust
//   OK      rest position done -> press position; OK again saves both
//   ≡       cancel and keep the old positions
// Goes back to rest and cancels if no pad is touched for 20 s.
// While the press position is being set the arm holds the manual button, so
// the grinder runs: unplug it or keep a cup under it.
bool ServoSetupActive();
void ServoSetupStart(Adafruit_SSD1306& display);
void ServoSetupUpdate(Adafruit_SSD1306& display, bool cancel, bool up, bool down, bool ok);
