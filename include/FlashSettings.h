#pragma once
// Settings kept in flash. Flash reads back as all zeros after a firmware
// upload, so 0 means "not set" for every field and "valid" only says the
// structure has been saved at least once.
typedef struct {
  boolean valid;
  float calibrationValue;  // load cell factor, raw counts per gram
  uint16_t calMassGrams;   // reference weight used for the last panel calibration
  uint16_t servoRestUs;    // servo pulse with the arm clear of the grinder button
  uint16_t servoPressUs;   // servo pulse with the arm holding the button down
} FlashSettings;
