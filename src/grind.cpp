/**
 grind.cpp - grind by weight
**/

#include <Arduino.h>
#include "grind.h"
#include "grinderservo.h"
#include "FlashStore.h"
#include "beep.h"

namespace {

enum class State { Idle, Zeroing, Taring, Grinding, Settling };

State state = State::Idle;
HX711_ADC* scale = nullptr;
bool scaleOk = true;

int selected = 1;
int doseDg[3] = {0, DOSE1_DEFAULT_DG, DOSE2_DEFAULT_DG};
// Each dose learns its own offset: they're often different beans, which
// grind at different rates, and a bigger dose can overshoot by more.
float offsetG[3] = {0.0f, OFFSET_DEFAULT_G, OFFSET_DEFAULT_G};  // [1], [2] used
bool offsetIsSet[3] = {false, false, false};
int grindDose = 1;  // the dose being ground (its preset and its offset)

float targetG = 0;
unsigned long stateSince = 0;
unsigned long pressStart = 0;
unsigned long releaseTime = 0;
unsigned long lastData = 0;
unsigned long lastProgress = 0;
unsigned long stableSince = 0;
float lastG = 0;
float peakG = 0;   // for the stall check: last level that counted as progress
float maxG = 0;    // highest reading this grind
int tareReadings = 0;
float stableRefG = 0;
int liftCount = 0;
GrindResult pendingResult = GrindResult::None;
bool learnFromThis = false;

GrindRecord last = {};
bool showLast = false;
unsigned long lastShownAt = 0;
bool finishedFlag = false;

float targetFor(int n) { return doseDg[n] / 10.0f; }

float clampF(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void saveGrindSettings() {
  settings.dose1Dg = doseDg[1];
  settings.dose2Dg = doseDg[2];
  settings.selectedDose = selected;
  for (int n = 1; n <= 2; n++) {
    settings.offsetSet[n - 1] = offsetIsSet[n] ? 1 : 0;
    settings.offsetCg[n - 1] = (int16_t)lroundf(offsetG[n] * 100.0f);
  }
  ServoSaveSettingsAtRest();  // after a grind, once the servo has stopped getting pulses
}

void restoreSamples() {
  if (scale) scale->setSamplesInUse(IDLE_SAMPLES);
}

void showResult(GrindResult r, float actual, float seconds) {
  last.dose = grindDose;
  last.targetG = targetG;
  last.actualG = actual;
  last.seconds = seconds;
  last.offsetG = offsetG[grindDose];
  last.newOffsetG = offsetG[grindDose];
  last.learned = false;
  last.result = r;
  showLast = true;
  lastShownAt = millis();
}

// The grinder never ran: back to idle, nothing to log
void abortBeforePress(GrindResult r) {
  restoreSamples();
  state = State::Idle;
  showResult(r, 0.0f, 0.0f);
  last.count = settings.grindCount;
  Serial.print("Grind not started: ");
  Serial.println(GrindResultName(r));
}

void finish(float actual) {
  restoreSamples();
  float seconds = (releaseTime - pressStart) / 1000.0f;
  showResult(pendingResult, actual, seconds);
  if (pendingResult == GrindResult::Done && learnFromThis) {
    float miss = actual - targetG;
    if (fabsf(miss) <= OFFSET_LEARN_MAX_ERROR_G) {
      offsetG[grindDose] = clampF(offsetG[grindDose] + OFFSET_LEARN_RATE * miss, 0.0f, OFFSET_MAX_G);
      offsetIsSet[grindDose] = true;
      last.learned = true;
    }
  }
  last.newOffsetG = offsetG[grindDose];
  settings.grindCount++;
  last.count = settings.grindCount;
  saveGrindSettings();
  state = State::Idle;
  finishedFlag = true;
  Serial.print("Grind ");
  Serial.print(GrindResultName(last.result));
  Serial.print(": ");
  Serial.print(actual, 2);
  Serial.print(" g of ");
  Serial.print(targetG, 1);
  Serial.print(" g in ");
  Serial.print(seconds, 1);
  Serial.print(" s, offset ");
  Serial.print(last.offsetG, 2);
  Serial.print(" -> ");
  Serial.println(offsetG[grindDose], 2);
}

// Let go of the button
void release(GrindResult r) {
  ServoRest();
  releaseTime = millis();
  pendingResult = r;
  learnFromThis = (r == GrindResult::Done);
  if (r == GrindResult::CupLifted) {
    finish(maxG);  // what was in the cup before it went
    return;
  }
  if (r == GrindResult::ScaleError) {
    finish(lastG);  // the last reading there was
    return;
  }
  state = State::Settling;
  stateSince = releaseTime;
  stableSince = releaseTime;
  stableRefG = lastG;
}

}  // namespace

void GrindBegin(bool ok) {
  scaleOk = ok;
  if (!settings.valid) return;
  if (settings.dose1Dg >= DOSE_MIN_DG && settings.dose1Dg <= DOSE_MAX_DG) doseDg[1] = settings.dose1Dg;
  if (settings.dose2Dg >= DOSE_MIN_DG && settings.dose2Dg <= DOSE_MAX_DG) doseDg[2] = settings.dose2Dg;
  if (settings.selectedDose == 1 || settings.selectedDose == 2) selected = settings.selectedDose;
  for (int n = 1; n <= 2; n++) {
    if (settings.offsetSet[n - 1]) {
      offsetG[n] = clampF(settings.offsetCg[n - 1] / 100.0f, 0.0f, OFFSET_MAX_G);
      offsetIsSet[n] = true;
    }
  }
}

bool GrindStart(HX711_ADC& lc) {
  if (state != State::Idle) return false;
  scale = &lc;
  if (!scaleOk) {
    beepTimes(3);
    return false;
  }
  grindDose = selected;
  targetG = targetFor(grindDose);
  showLast = false;
  // A shorter moving average while grinding means less lag. getData() first,
  // because setSamplesInUse() seeds the new average with the last value read.
  // The tare is done here once the short average has refilled with fresh
  // readings (GrindUpdate), rather than with the library's tareNoDelay(),
  // which always waits for its full 18-sample buffer (~1.9 s).
  lc.getData();
  lc.setSamplesInUse(GRIND_SAMPLES);
  tareReadings = 0;
  state = State::Taring;
  stateSince = millis();
  beep();
  Serial.print("Grind to ");
  Serial.print(targetG, 1);
  Serial.print(" g (dose ");
  Serial.print(grindDose);
  Serial.print("), offset ");
  Serial.println(offsetG[grindDose], 2);
  return true;
}

void GrindStop() {
  if (state == State::Taring) {
    abortBeforePress(GrindResult::Stopped);
  } else if (state == State::Grinding) {
    release(GrindResult::Stopped);
  }
}

void GrindZero(HX711_ADC& lc) {
  if (state != State::Idle) return;
  scale = &lc;
  showLast = false;
  lc.tareNoDelay();
  state = State::Zeroing;
  stateSince = millis();
}

bool GrindUpdate(HX711_ADC& lc, bool newData, float weight, bool tareDone) {
  finishedFlag = false;
  scale = &lc;
  unsigned long now = millis();
  if (newData) {
    lastG = weight;
    lastData = now;
  }

  switch (state) {
    case State::Idle:
      if (showLast && now - lastShownAt >= GRIND_RESULT_SHOW_MS) showLast = false;
      break;

    case State::Zeroing:
      if (tareDone || now - stateSince >= ZERO_TIMEOUT_MS) state = State::Idle;
      break;

    case State::Taring:
      if (newData && ++tareReadings >= GRIND_TARE_READINGS) {
        // Zero on the current (short-average) reading: move the library's
        // tare offset by it, in raw counts
        lc.setTareOffset(lc.getTareOffset() + lroundf(weight * lc.getCalFactor()));
        ServoPress();
        state = State::Grinding;
        pressStart = stateSince = lastProgress = lastData = now;
        lastG = peakG = maxG = 0.0f;
        liftCount = 0;
      } else if (now - stateSince >= GRIND_TARE_TIMEOUT_MS) {
        abortBeforePress(GrindResult::ScaleError);
        beepTimes(3);
      }
      break;

    case State::Grinding:
      if (now - lastData >= GRIND_DATA_TIMEOUT_MS) {
        release(GrindResult::ScaleError);
        break;
      }
      if (now - pressStart >= GRIND_MAX_MS) {
        release(GrindResult::MaxTime);
        break;
      }
      if (newData) {
        if (weight >= targetG - offsetG[grindDose]) {
          release(GrindResult::Done);
          break;
        }
        if (weight > maxG) maxG = weight;
        if (weight > peakG + GRIND_STALL_G) {
          peakG = weight;
          lastProgress = now;
        }
        // Two readings in a row, so one bump or vibration spike doesn't count
        if (weight < peakG - GRIND_CUP_LIFT_G) {
          if (++liftCount >= 2) {
            release(GrindResult::CupLifted);
            break;
          }
        } else {
          liftCount = 0;
        }
      }
      if (now - lastProgress >= GRIND_STALL_MS) release(GrindResult::NoFlow);
      break;

    case State::Settling:
      if (newData) {
        if (weight < stableRefG - GRIND_CUP_LIFT_G) {
          // Cup taken away before it settled: keep the last steady reading,
          // but it may still have been rising, so don't learn from it
          learnFromThis = false;
          finish(stableRefG);
          break;
        }
        if (fabsf(weight - stableRefG) > SETTLE_STABLE_G) {
          stableRefG = weight;
          stableSince = now;
        }
      }
      if ((now - releaseTime >= SETTLE_MIN_MS && now - stableSince >= SETTLE_STABLE_MS) ||
          now - releaseTime >= SETTLE_MAX_MS) {
        finish(lastG);
      }
      break;
  }
  return finishedFlag;
}

bool GrindRunning() { return state == State::Taring || state == State::Grinding; }
bool GrindBusy() { return state != State::Idle; }
const GrindRecord& GrindLastRecord() { return last; }

void GrindSelectDose(int n) {
  if (n != 1 && n != 2) return;
  selected = n;
  showLast = false;
}

int GrindSelectedDose() { return selected; }
int GrindDoseDg(int n) { return (n == 1 || n == 2) ? doseDg[n] : 0; }

void GrindSetDoseDg(int n, int dg) {
  if (n != 1 && n != 2) return;
  if (dg < DOSE_MIN_DG) dg = DOSE_MIN_DG;
  if (dg > DOSE_MAX_DG) dg = DOSE_MAX_DG;
  doseDg[n] = dg;
  saveGrindSettings();
}

float GrindOffsetG(int n) { return (n == 1 || n == 2) ? offsetG[n] : 0.0f; }

void GrindSetOffsetG(int n, float g) {
  if (n != 1 && n != 2) return;
  offsetG[n] = clampF(g, 0.0f, OFFSET_MAX_G);
  offsetIsSet[n] = true;
  saveGrindSettings();
}

String GrindStatus() {
  char buf[24];
  if (!scaleOk) return String("Load cell error!");
  switch (state) {
    case State::Zeroing:
      return String("Zeroing...");
    case State::Taring:
      return String("Taring...");
    case State::Grinding:
      snprintf(buf, sizeof buf, "Grind to %.1fg", targetG);
      return String(buf);
    case State::Settling:
      return String("Settling...");
    case State::Idle:
      break;
  }
  if (showLast) {
    switch (last.result) {
      case GrindResult::Done:
        snprintf(buf, sizeof buf, "Done %.1f (%+.1f)", last.actualG, last.actualG - last.targetG);
        return String(buf);
      case GrindResult::Stopped:
        if (last.seconds == 0.0f) return String("Stopped");
        snprintf(buf, sizeof buf, "Stopped %.1fg", last.actualG);
        return String(buf);
      case GrindResult::NoFlow:
        return String("No flow: hopper?");
      case GrindResult::CupLifted:
        return String("Cup lifted");
      case GrindResult::MaxTime:
        snprintf(buf, sizeof buf, "Max time %.1fg", last.actualG);
        return String(buf);
      case GrindResult::ScaleError:
        return String("Scale error");
      case GrindResult::None:
        break;
    }
  }
  snprintf(buf, sizeof buf, "Dose %d: %.1fg", selected, targetFor(selected));
  return String(buf);
}

String GrindStatusRight() {
  char buf[12];
  float seconds;
  if (state == State::Grinding) {
    seconds = (millis() - pressStart) / 1000.0f;
  } else if (state == State::Settling) {
    seconds = (releaseTime - pressStart) / 1000.0f;
  } else if (state == State::Idle && showLast && last.seconds > 0.0f) {
    seconds = last.seconds;
  } else {
    return String("");
  }
  snprintf(buf, sizeof buf, "%.1fs", seconds);
  return String(buf);
}

const char* GrindResultName(GrindResult r) {
  switch (r) {
    case GrindResult::Done: return "done";
    case GrindResult::Stopped: return "stopped";
    case GrindResult::NoFlow: return "no flow";
    case GrindResult::CupLifted: return "cup lifted";
    case GrindResult::MaxTime: return "max time";
    case GrindResult::ScaleError: return "scale error";
    case GrindResult::None: break;
  }
  return "none";
}
