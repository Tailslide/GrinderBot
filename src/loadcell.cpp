/**
 loadcell.cpp - load cell start-up and calibration storage
 (calibrating is done from the panel: panelcal.cpp)
**/

#include <Arduino.h>
#include <HX711_ADC.h>
#include "FlashStore.h"
#include "loadcell.h"

// Save the calibration factor to flash so it survives a power cycle.
// (Uploading new firmware still wipes it; see DEFAULT_CAL_FACTOR.)
void saveCalFactor(float calFactor) {
  settings.calibrationValue = calFactor;
  SaveSettings();
}

bool SetupLoadCell(HX711_ADC& LoadCell, String& message)
{
  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Load cell timeout: check the HX711 wiring (DOUT D6, SCK D7) and its power");
    message = "Load cell?\r\nCheck wires";
    return false;
  }
  // Use the saved factor if there is one, otherwise the default from loadcell.h.
  // A 0 factor means only other settings were saved, so it isn't a calibration.
  bool haveSavedCal = settings.valid && settings.calibrationValue != 0.0f;
  float calFactor = haveSavedCal ? settings.calibrationValue : DEFAULT_CAL_FACTOR;
  LoadCell.setCalFactor(calFactor);
  Serial.println("Startup is complete");
  // Wait for the first reading, but not forever if the HX711 stops answering
  unsigned long start = millis();
  while (!LoadCell.update()) {
    if (millis() - start > 1000) {
      Serial.println("Load cell stopped answering after start-up");
      message = "Load cell?\r\nCheck wires";
      return false;
    }
  }
  if (haveSavedCal) {
    message = "Loaded Calib.";
  } else if (DEFAULT_CAL_FACTOR != 1.0f) {
    message = "Default Calib.";
  } else {
    message = "No Calib.\r\nStored";
  }
  Serial.print(message);
  Serial.print(" (factor ");
  Serial.print(calFactor, 4);
  Serial.println(")");
  return true;
}
