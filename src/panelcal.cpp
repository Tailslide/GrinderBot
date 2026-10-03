/**
 panelcal.cpp - calibrate the scale from the touch pads and screen
**/

#include <Arduino.h>
#include "panelcal.h"
#include "display.h"
#include "loadcell.h"
#include "FlashStore.h"
#include "ui.h"

namespace {

enum class Step { Idle, Empty, Taring, Place, Confirm };

const int REF_MIN_G = 100;
const int REF_MAX_G = 1000;  // the load cell is rated 1 kg
const int REF_STEP_G = 100;
// Calibration factors are raw counts per gram, typically hundreds or more.
// Anything this small means there was nothing on the scale when measured.
const float MIN_CAL_FACTOR = 10.0f;
const unsigned long CONFIRM_REDRAW_MS = 150;

Step step = Step::Idle;
float oldCalFactor = 1.0f;
float newCalFactor = 1.0f;
int refGrams = REF_MIN_G;
unsigned long lastDraw = 0;

void showPlace(Adafruit_SSD1306& display) {
  String text = "Put " + String(refGrams) + " g\r\n^v adj, OK";
  DisplayMessage(display, text);
}

void cancel(HX711_ADC& LoadCell, Adafruit_SSD1306& display) {
  LoadCell.setCalFactor(oldCalFactor);
  step = Step::Idle;
  Serial.println("Calibration cancelled");
  beep();
  DisplayMessage(display, "Cancelled", 1000);
}

}  // namespace

bool PanelCalActive() {
  return step != Step::Idle;
}

void PanelCalStart(HX711_ADC& LoadCell, Adafruit_SSD1306& display) {
  oldCalFactor = LoadCell.getCalFactor();
  // Start from the weight used last time, if there is one
  int last = settings.valid ? settings.calMassGrams : 0;
  refGrams = (last >= REF_MIN_G && last <= REF_MAX_G) ? last : REF_MIN_G;
  step = Step::Empty;
  Serial.println("Panel calibration started");
  DisplayMessage(display, "Empty scale\r\nthen OK");
}

void PanelCalUpdate(HX711_ADC& LoadCell, Adafruit_SSD1306& display,
                    bool cancelPressed, bool up, bool down, bool ok, bool tareDone) {
  if (step == Step::Idle) return;
  if (cancelPressed) {
    cancel(LoadCell, display);
    return;
  }

  switch (step) {
    case Step::Empty:
      if (ok) {
        beep();
        LoadCell.tareNoDelay();
        step = Step::Taring;
        DisplayMessage(display, "Taring...");
      }
      break;

    case Step::Taring:
      if (tareDone) {
        beep();
        step = Step::Place;
        showPlace(display);
      }
      break;

    case Step::Place:
      if (up && refGrams < REF_MAX_G) {
        refGrams += REF_STEP_G;
        showPlace(display);
      } else if (down && refGrams > REF_MIN_G) {
        refGrams -= REF_STEP_G;
        showPlace(display);
      } else if (ok) {
        DisplayMessage(display, "Measuring\r\nhold still");
        LoadCell.refreshDataSet();  // ~2 s: refills the averaging buffer with the weight on
        float factor = LoadCell.getNewCalibration((float)refGrams);  // also applies it
        if (fabs(factor) < MIN_CAL_FACTOR) {
          LoadCell.setCalFactor(oldCalFactor);
          beep();
          DisplayMessage(display, "No weight?\r\nTry again", 1500);
          showPlace(display);
        } else {
          newCalFactor = factor;
          beep();
          step = Step::Confirm;
          lastDraw = 0;
        }
      }
      break;

    case Step::Confirm:
      if (ok) {
        settings.calMassGrams = refGrams;
        saveCalFactor(newCalFactor);
        Serial.print("Calibration saved, factor ");
        Serial.print(newCalFactor, 4);
        Serial.println(" (put this in DEFAULT_CAL_FACTOR to keep it across uploads)");
        beep();
        step = Step::Idle;
        DisplayMessage(display, "Saved", 1000);
      } else if (millis() - lastDraw >= CONFIRM_REDRAW_MS) {
        // Live reading with the new factor, so you can check it before saving
        lastDraw = millis();
        String text = String(LoadCell.getData(), 1) + " g\r\nOK to save";
        DisplayMessage(display, text);
      }
      break;

    default:
      break;
  }
}
