/**
 loadcell.cpp - load cell functions
**/

#include <Arduino.h>
#include <HX711_ADC.h>
#include "FlashStore.h"
#include <functional>
#include "loadcell.h"

// Save the calibration factor to flash so it survives a power cycle.
// (Uploading new firmware still wipes it; see DEFAULT_CAL_FACTOR.)
void saveCalFactor(float calFactor) {
  settings.calibrationValue = calFactor;
  SaveSettings();
}

void calibrate(HX711_ADC& LoadCell, DisplayCallback display) {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");
  display("Starting\r\nCalib.");
  delay(1000);
  display("Empty scale\r\nSerial: t");
  
  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue, 4);
  Serial.println(", also put this in DEFAULT_CAL_FACTOR (loadcell.h) so it survives firmware uploads.");
  Serial.println("Save this value to flash? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        saveCalFactor(newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue, 4);
        Serial.println(" saved to flash");
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to flash");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}

void changeSavedCalFactor(HX711_ADC& LoadCell) {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.println("Save this value to flash? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        saveCalFactor(newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue, 4);
        Serial.println(" saved to flash");
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to flash");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}

bool SetupLoadCell(HX711_ADC& LoadCell, String& message)
{
  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    message = "ERROR:\r\nLoad Cell Wiring?";
    return false;
    //while (1);
  }
  else {
    // Use the saved factor if there is one, otherwise the default from loadcell.h.
    // (float, not int: an int would cut off the decimals of the factor.)
    // A 0 factor means only other settings were saved, so it isn't a calibration.
    bool haveSavedCal = settings.valid && settings.calibrationValue != 0.0f;
    float calFactor = haveSavedCal ? settings.calibrationValue : DEFAULT_CAL_FACTOR;
    LoadCell.setCalFactor(calFactor);
    Serial.println("Startup is complete");
    while (!LoadCell.update());
    if (haveSavedCal)
    {
        message = "Loaded Calib.";
    }
    else if (DEFAULT_CAL_FACTOR != 1.0f)
    {
        message = "Default Calib.";
    }
    else
    {
        message = "No Calib.\r\nStored";
    }
    Serial.print(message);
    Serial.print(" (factor ");
    Serial.print(calFactor, 4);
    Serial.println(")");
  }
  return true;
}
