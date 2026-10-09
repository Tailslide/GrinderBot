#pragma once
#include <Arduino.h>
#include <HX711_ADC.h>
#include "config.h"  // setup-specific defaults: doses, max time, starting offset, stall time

// Grind by weight.
//
// OK on the weight screen: tare (with the cup on), ease the servo onto the
// grinder's manual button and hold it, then let go when the cup reaches the
// target minus an offset. The offset covers what still lands after letting go
// (grounds in the chute, the motor spinning down, the scale's averaging lag),
// and is learned: after each normal grind it moves halfway towards the miss.
// Dose 1 and dose 2 each learn their own offset (often different beans).
// Any pad stops a grind. Safety stops let go of the button if the weight stops
// rising, the cup is lifted, the load cell goes quiet or the grind runs long,
// and the watchdog (watchdog.h) covers a hung firmware.

// --- Tuning ---
const int GRIND_SAMPLES = 4;    // HX711_ADC moving average while grinding (less lag)
const int IDLE_SAMPLES = 16;    // library default, steadier reading when idle
const int GRIND_TARE_READINGS = GRIND_SAMPLES + 3;  // fresh readings before zeroing (~0.7 s):
                                                    //   the average plus the 2 the library drops, +1
const unsigned long GRIND_TARE_TIMEOUT_MS = 3000;
const int GRIND_MAX_MIN_S = 10;                    // range for Menu -> Max time (the default,
const int GRIND_MAX_MAX_S = 600;                   //   GRIND_MAX_DEFAULT_S, is in config.h)
// let go if the weight hasn't risen (GRIND_STALL_MS, in config.h)
const float GRIND_STALL_G = 0.3f;                  //   by this much in GRIND_STALL_MS (empty hopper, clog)
const float GRIND_CUP_LIFT_G = 5.0f;               // let go if the weight drops this far below its peak
const unsigned long GRIND_DATA_TIMEOUT_MS = 1000;  // let go if the load cell stops sending readings
const unsigned long SETTLE_MIN_MS = 1500;          // after letting go, wait at least this long,
const unsigned long SETTLE_STABLE_MS = 1000;       //   then for the reading to hold still this long
const float SETTLE_STABLE_G = 0.15f;               //   (within this much),
const unsigned long SETTLE_MAX_MS = 6000;          //   but no longer than this
const float OFFSET_MAX_G = 5.0f;
const float OFFSET_LEARN_RATE = 0.5f;
const float OFFSET_LEARN_MAX_ERROR_G = 3.0f;       // bigger misses are probably a bumped cup: not learned
const int DOSE_MIN_DG = 10;                        // dose presets, 0.1 g units: 1.0 - 99.9 g
const int DOSE_MAX_DG = 999;
const unsigned long GRIND_RESULT_SHOW_MS = 60000;  // result stays on the status line this long
const unsigned long ZERO_TIMEOUT_MS = 4000;

enum class GrindResult : uint8_t { None, Done, Stopped, NoFlow, CupLifted, MaxTime, ScaleError };

struct GrindRecord {
  uint8_t dose;        // 1 or 2
  float targetG;
  float actualG;       // settled weight in the cup
  float seconds;       // how long the button was held
  float offsetG;       // offset used for this grind
  float newOffsetG;    // offset after learning from it
  bool learned;        // whether this grind updated the offset
  GrindResult result;
  uint32_t count;      // grinds so far (kept in flash)
};

void GrindBegin(bool scaleOk);  // after settings are loaded; scaleOk = load cell started
bool GrindStart(HX711_ADC& lc); // tare, then press; false if it can't start
void GrindStop();               // a pad was touched
void GrindZero(HX711_ADC& lc);  // tare without grinding (≡ tap)
// Call every loop. Returns true once, when a grind has finished (servo back at
// rest, weight settled); GrindLastRecord() then has the result.
bool GrindUpdate(HX711_ADC& lc, bool newData, float weight, bool tareDone);
bool GrindRunning();  // taring for a grind, or grinding: a touch stops it
bool GrindBusy();     // running, settling or zeroing
const GrindRecord& GrindLastRecord();

void GrindSelectDose(int n);  // 1 or 2
int GrindSelectedDose();
int GrindDoseDg(int n);
void GrindSetDoseDg(int n, int dg);  // and save
float GrindOffsetG(int n);            // learned offset for dose 1 or 2
void GrindSetOffsetG(int n, float g);  // and save
int GrindMaxS();                       // longest the button is held, seconds
void GrindSetMaxS(int s);              // and save

String GrindStatus();       // left of the status line
String GrindStatusRight();  // right of the status line (a time), or ""
const char* GrindResultName(GrindResult r);  // short lower-case name, for logs and MQTT
