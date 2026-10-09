#pragma once
#include <stdint.h>
// Settings kept in flash. Flash reads back as all zeros after a firmware
// upload, so 0 means "not set" for every field (the code falls back to a
// default) and "valid" only says the structure has been saved at least once.
typedef struct {
  boolean valid;
  float calibrationValue;  // load cell factor, raw counts per gram
  uint16_t calMassGrams;   // reference weight used for the last panel calibration
  uint16_t servoRestUs;    // servo pulse with the arm clear of the grinder button
  uint16_t servoPressUs;   // servo pulse with the arm holding the button down
  uint16_t dose1Dg;        // dose presets on the 1 / 2 pads, in 0.1 g
  uint16_t dose2Dg;
  uint8_t selectedDose;    // 1 or 2
  uint8_t offsetSet[2];    // per dose (1, 2): 1 once its grind offset has been learned or entered
  int16_t offsetCg[2];     // per dose: grind offset in 0.01 g (only if offsetSet)
  uint32_t grindCount;     // grinds that ran the grinder, for Home Assistant
  uint16_t grindMaxS;      // longest the button is held, seconds
} FlashSettings;
