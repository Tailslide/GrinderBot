#pragma once
// Settings kept in flash. "valid" is set to true once the structure has been
// saved for the first time (flash reads back as all zeros after an upload).
typedef struct {
  boolean valid;
  float calibrationValue;
  uint16_t calMassGrams;  // reference weight used for the last panel calibration
} FlashSettings;
