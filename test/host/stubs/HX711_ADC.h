#pragma once
#include "Arduino.h"
// Stand-in for the HX711_ADC library. The test drives the weight itself;
// this records what the firmware asked of the library.
class HX711_ADC {
 public:
  float cal = 1.0f; long rawCountsAboveTare = 0; int tares = 0; int samples = 16;
  unsigned long tareRequestedAt = 0; bool tareRequested = false;
  float getCalFactor() { return cal; }
  void setCalFactor(float f) { cal = f; }
  void tareNoDelay() { tares++; rawCountsAboveTare = 0; tareRequested = true; tareRequestedAt = millis(); }
  bool refreshDataSet() { return true; }
  float getData() { return rawCountsAboveTare / cal; }
  float getNewCalibration(float m) { float f = (getData() * cal) / m; cal = f; return f; }
  void setSamplesInUse(int n) { samples = n; }
  int getSamplesInUse() { return samples; }
};
