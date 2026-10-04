// Host-side tests for the touch-pad UI, the settings screens and grind by
// weight. The hardware is replaced by stand-ins (stubs/) and a simulated
// grinder: while the servo holds the button, grounds land at a set rate, and
// some still land after it lets go. Run with ./run.sh from this folder.
#include <algorithm>
#include <iostream>
#include <vector>
#include "pads.h"
#include "ui.h"
#include "grind.h"
#include "grinderservo.h"
#include "watchdog.h"
#include "net.h"
#include "display.h"
#include "FlashStore.h"

unsigned long fakeMillis = 1000;
int fakePins[8] = {0};
int fakeDigitalWrites[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
FakeSerial Serial;
std::string lastScreen;
FakeStore flash_store;
FlashSettings settings{};
int fakeServoUs = -1, fakeServoAttached = 0, fakeServoFirstUs = -1;
bool fakeServoActive = false;
int beeps = 0, lastBeepTimes = 0;
void beep() { beeps++; }
void beepTimes(int n) { lastBeepTimes = n; }
void SaveSettings() { settings.valid = true; flash_store.write(settings); }      // as FlashSettings.cpp
void saveCalFactor(float f) { settings.calibrationValue = f; SaveSettings(); }  // as loadcell.cpp

// watchdog.h
bool wdArmed = false;
void WatchdogArm() { wdArmed = true; }
void WatchdogDisarm() { wdArmed = false; }
void WatchdogFeed() {}
bool WatchdogCausedLastReset() { return false; }

// net.h
std::vector<GrindRecord> published;
void NetBegin() {}
void NetUpdate(bool, bool) {}
void NetPublishGrind(const GrindRecord& r) { published.push_back(r); }
bool NetConnected() { return false; }
String NetInfo() { return String("WiFi test\nMQTT test\nID test"); }

HX711_ADC lc;
Adafruit_SSD1306 disp;
Pad menu(0), up(1), down(2), ok(3);
UiAction last = UiAction::None;

// --- Simulated scale and grinder ---
float rawG = 0;          // what's on the scale, grams
float tareG = 0;         // where the scale reads zero
bool dataOn = true;      // load cell sending readings, 10 a second
bool tareWorks = true;   // tares finish 600 ms after they're asked for
float flowGps = 1.2f;    // grounds per second while the button is held
float inflightG = 0.8f;  // grounds that still land after letting go, over 0.5 s
float inflightLeft = 0;
unsigned long nextData = 0;
int finished = 0;

bool grinderRunning() { return fakeServoActive && fakeServoUs >= ServoPressUs() - 5; }

void loopOnce() {  // mirrors main.cpp's loop
  menu.update(); up.update(); down.update(); ok.update();
  if (grinderRunning()) {
    rawG += flowGps * 0.01f;
    inflightLeft = inflightG;
  } else if (inflightLeft > 0) {
    float d = std::min(inflightLeft, inflightG * 0.02f);
    rawG += d;
    inflightLeft -= d;
  }
  bool newData = false;
  if (dataOn && (long)(fakeMillis - nextData) >= 0) {
    newData = true;
    nextData = fakeMillis + 100;
  }
  bool tareDone = false;
  if (lc.tareRequested && tareWorks && fakeMillis - lc.tareRequestedAt >= 600) {
    lc.tareRequested = false;
    tareG = rawG;
    tareDone = true;
  }
  if (GrindUpdate(lc, newData, rawG - tareG, tareDone)) {
    finished++;
    const GrindRecord& r = GrindLastRecord();
    beepTimes(r.result == GrindResult::Done || r.result == GrindResult::Stopped ? 2 : 3);
    NetPublishGrind(r);
  }
  last = UiUpdate(lc, disp, menu, up, down, ok, tareDone);
  ServoUpdate();
  fakeMillis += 10;
}
void touch(int pin, unsigned long ms) { fakePins[pin] = 1; for (unsigned long t = 0; t < ms; t += 10) loopOnce(); fakePins[pin] = 0; loopOnce(); }
void idle(unsigned long ms) { for (unsigned long t = 0; t < ms; t += 10) loopOnce(); }
template <class F> bool runUntil(F done, unsigned long maxMs) {
  for (unsigned long t = 0; t < maxMs; t += 10) { if (done()) return true; loopOnce(); }
  return done();
}
std::string status() { return GrindStatus().s; }
float reading() { return rawG - tareG; }

#define CHECK(c) do { if (!(c)) { std::cerr << "FAIL line " << __LINE__ << ": " #c "  status=[" << status() \
  << "] screen=[" << lastScreen << "] servo=" << fakeServoUs << " reading=" << reading() << "\n"; return 1; } } while (0)

// Put a cup on, press OK, and run until the grind has finished
int grindOnce(float cupG = 300.0f) {
  rawG = cupG;
  idle(300);
  int before = finished;
  touch(3, 100);
  if (!runUntil([&] { return finished > before; }, 90000)) return -1;
  idle(50);
  return 0;
}

int main() {
  // --- Boot ---
  ServoBegin(4);
  GrindBegin(true);
  CHECK(fakeServoFirstUs == 1024); CHECK(fakeServoAttached == 1);  // parked before attach
  CHECK(ServoRestUs() == 1024 && ServoPressUs() == 1292);
  CHECK(!wdArmed);
  CHECK(status() == "Dose 1: 9.0g");
  idle(1000);
  CHECK(!fakeServoActive); CHECK(fakeDigitalWrites[4] == LOW);  // detached at rest, line low
  CHECK(last == UiAction::ShowWeight);

  // --- Dose selection ---
  touch(2, 100); CHECK(status() == "Dose 2: 18.0g"); CHECK(GrindSelectedDose() == 2);
  touch(1, 100); CHECK(status() == "Dose 1: 9.0g");
  touch(2, 100);

  // --- A normal grind ---
  rawG = 300; idle(300);
  touch(3, 100);
  CHECK(status() == "Taring..."); CHECK(lc.samples == GRIND_SAMPLES);
  CHECK(runUntil([] { return wdArmed; }, 1000));             // tare done -> pressing
  idle(120); CHECK(fakeServoUs > 1024 && fakeServoUs < 1292);  // easing onto the button
  idle(200); CHECK(fakeServoUs == 1292); CHECK(grinderRunning());
  CHECK(status() == "Grind to 18.0g");
  CHECK(runUntil([] { return !grinderRunning(); }, 30000));
  CHECK(fakeServoUs == 1024); CHECK(!wdArmed);                // let go, watchdog off
  CHECK(reading() >= 17.0f && reading() < 17.2f);             // at target - 1.0 g offset
  CHECK(status() == "Settling...");
  CHECK(runUntil([] { return finished == 1; }, 7000));
  const GrindRecord& r1 = GrindLastRecord();
  CHECK(r1.result == GrindResult::Done); CHECK(r1.dose == 2); CHECK(r1.targetG == 18.0f);
  CHECK(r1.actualG > 17.7f && r1.actualG < 18.0f);            // the in-flight 0.8 g landed
  CHECK(r1.seconds > 14.0f && r1.seconds < 15.0f);
  CHECK(r1.learned); CHECK(r1.offsetG == 1.0f);
  CHECK(r1.newOffsetG < 1.0f && r1.newOffsetG > 0.85f);       // halfway towards the miss
  CHECK(r1.count == 1); CHECK(lastBeepTimes == 2);
  CHECK(published.size() == 1);
  CHECK(lc.samples == IDLE_SAMPLES);
  CHECK(settings.grindCount == 1 && settings.offsetSet == 1 && settings.selectedDose == 2);
  CHECK(settings.offsetCg == (int16_t)lroundf(r1.newOffsetG * 100));
  CHECK(status().find("Done 17.") == 0);
  char secs[16]; snprintf(secs, sizeof secs, "%.1fs", r1.seconds); CHECK(GrindStatusRight().s == secs);
  idle(1000); CHECK(!fakeServoActive);                        // detached again

  // --- The offset converges over a few grinds ---
  for (int i = 0; i < 6; i++) CHECK(grindOnce() == 0);
  CHECK(GrindLastRecord().result == GrindResult::Done);
  CHECK(fabsf(GrindLastRecord().actualG - 18.0f) < 0.15f);
  CHECK(GrindOffsetG() > 0.7f && GrindOffsetG() < 0.95f);
  float learnedOffset = GrindOffsetG();
  int writes = flash_store.writes;

  // --- Touching a pad stops a grind straight away ---
  rawG = 300; idle(300);
  touch(3, 100);
  CHECK(runUntil([] { return grinderRunning(); }, 1500));
  idle(3000);
  fakePins[1] = 1; loopOnce();
  CHECK(fakeServoUs == 1024); CHECK(!wdArmed);
  idle(100); fakePins[1] = 0; loopOnce();
  CHECK(runUntil([] { return finished == 8; }, 7000));
  CHECK(GrindLastRecord().result == GrindResult::Stopped); CHECK(!GrindLastRecord().learned);
  CHECK(GrindOffsetG() == learnedOffset); CHECK(GrindSelectedDose() == 2);  // the touch did nothing else
  CHECK(status().find("Stopped ") == 0);
  int tares = lc.tares;
  idle(500); CHECK(!GrindBusy()); CHECK(lc.tares == tares);   // letting go didn't start a grind
  CHECK(flash_store.writes == writes + 1);

  // --- Empty hopper: no flow ---
  flowGps = 0;
  CHECK(grindOnce() == 0);
  CHECK(GrindLastRecord().result == GrindResult::NoFlow);
  CHECK(GrindLastRecord().seconds >= 5.0f && GrindLastRecord().seconds < 5.3f);
  CHECK(lastBeepTimes == 3); CHECK(status() == "No flow: hopper?");
  CHECK(GrindOffsetG() == learnedOffset);
  flowGps = 1.2f;

  // --- Cup lifted mid-grind ---
  rawG = 300; idle(300);
  touch(3, 100);
  CHECK(runUntil([] { return grinderRunning(); }, 1500));
  idle(3000);
  rawG -= 300;
  CHECK(runUntil([] { return !grinderRunning(); }, 400));     // two low readings
  CHECK(GrindLastRecord().result == GrindResult::CupLifted); CHECK(finished == 10);
  CHECK(status() == "Cup lifted"); CHECK(!GrindBusy());
  idle(1000);

  // --- Load cell goes quiet mid-grind ---
  rawG = 300; idle(300);
  touch(3, 100);
  CHECK(runUntil([] { return grinderRunning(); }, 1500));
  idle(2000);
  dataOn = false;
  CHECK(runUntil([] { return !grinderRunning(); }, 1300));
  CHECK(GrindLastRecord().result == GrindResult::ScaleError); CHECK(status() == "Scale error");
  dataOn = true; idle(1000);

  // --- Never holds the button longer than GRIND_MAX_MS ---
  flowGps = 0.1f;  // still rising, so not a stall
  CHECK(grindOnce() == 0);
  CHECK(GrindLastRecord().result == GrindResult::MaxTime);
  CHECK(GrindLastRecord().seconds >= 60.0f && GrindLastRecord().seconds < 60.2f);
  flowGps = 1.2f;
  CHECK(GrindOffsetG() == learnedOffset);

  // --- Stopped while taring: the grinder never runs, nothing logged ---
  size_t logged = published.size();
  uint32_t count = settings.grindCount;
  rawG = 300; idle(300);
  touch(3, 100); idle(200);
  touch(0, 100);
  CHECK(!GrindBusy()); CHECK(status() == "Stopped"); CHECK(lc.samples == IDLE_SAMPLES);
  idle(1000);
  CHECK(fakeServoUs == 1024); CHECK(published.size() == logged); CHECK(settings.grindCount == count);
  CHECK(lc.tares == tares + 4 + 1);  // the 4 grinds above, plus this one (not a ≡ tare on release)

  // --- A tare that never finishes ---
  tareWorks = false;
  touch(3, 100); idle(3100);
  CHECK(!GrindBusy()); CHECK(status() == "Scale error"); CHECK(lastBeepTimes == 3);
  CHECK(fakeServoUs == 1024); CHECK(published.size() == logged);
  tareWorks = true; lc.tareRequested = false;

  // --- Tap ≡ to tare ---
  tares = lc.tares;
  touch(0, 100); CHECK(lc.tares == tares + 1); CHECK(status() == "Zeroing...");
  idle(700); CHECK(!GrindBusy()); CHECK(status() == "Dose 2: 18.0g");

  // --- Menu: two items per screen, ▲/▼ wrap ---
  touch(0, 2100); CHECK(lastScreen == ">Calibrate\r\n Servo pos"); CHECK(last == UiAction::None);
  CHECK(lc.tares == tares + 1);  // the long press didn't also tare
  touch(2, 100); CHECK(lastScreen == " Calibrate\r\n>Servo pos");
  touch(2, 100); CHECK(lastScreen == ">Dose 1\r\n Dose 2");
  touch(2, 100); touch(2, 100); CHECK(lastScreen == ">Offset\r\n Network");
  touch(2, 100); CHECK(lastScreen == " Offset\r\n>Network");
  touch(2, 100); CHECK(lastScreen == ">Calibrate\r\n Servo pos");
  touch(1, 100); CHECK(lastScreen == " Offset\r\n>Network");
  touch(0, 100); idle(50); CHECK(last == UiAction::ShowWeight); CHECK(lc.tares == tares + 1);

  // --- Dose 1 from the menu ---
  writes = flash_store.writes;
  touch(0, 2100); touch(2, 100); touch(2, 100); touch(3, 100);
  CHECK(lastScreen == "Dose 1  9.0\r\n^v adj, OK");
  touch(1, 100); touch(1, 100); touch(1, 100); CHECK(lastScreen == "Dose 1  9.3\r\n^v adj, OK");
  touch(3, 100); CHECK(lastScreen == "Saved"); CHECK(GrindDoseDg(1) == 93);
  CHECK(settings.dose1Dg == 93); CHECK(flash_store.writes == writes + 1);
  idle(200); CHECK(!GrindBusy()); CHECK(lc.tares == tares + 1);  // letting go of OK didn't start a grind

  // --- Hold 2 to edit dose 2; the held pad doesn't run the number up ---
  fakePins[2] = 1; idle(2500);
  CHECK(lastScreen == "Dose 2 18.0\r\n^v adj, OK"); CHECK(GrindSelectedDose() == 2);
  fakePins[2] = 0; loopOnce();
  CHECK(lastScreen == "Dose 2 18.0\r\n^v adj, OK");
  touch(1, 100); CHECK(lastScreen == "Dose 2 18.1\r\n^v adj, OK");
  touch(0, 100); CHECK(lastScreen == "Cancelled"); CHECK(GrindDoseDg(2) == 180);
  // Hold ▲ to run it up quickly, then save
  fakePins[2] = 1; idle(2100); fakePins[2] = 0; loopOnce();
  touch(1, 1000); CHECK(lastScreen == "Dose 2 18.6\r\n^v adj, OK");  // press + 5 repeats
  touch(3, 100); CHECK(GrindDoseDg(2) == 186); CHECK(status() == "Dose 2: 18.6g");

  // --- Offset from the menu ---
  touch(0, 2100); for (int i = 0; i < 4; i++) touch(2, 100); touch(3, 100);
  int shown = (int)lroundf(learnedOffset * 10);
  char expect[40]; snprintf(expect, sizeof expect, "Offset  %d.%d\r\n^v adj, OK", shown / 10, shown % 10);
  CHECK(lastScreen == expect);
  touch(1, 100); touch(3, 100);
  CHECK(fabsf(GrindOffsetG() - (shown + 1) / 10.0f) < 0.001f);

  // --- Network screen; any pad closes it without doing anything else ---
  tares = lc.tares;
  touch(0, 2100); touch(1, 100); touch(3, 100);
  CHECK(lastScreen == "WiFi test\nMQTT test\nID test\nGrinds " + std::to_string(settings.grindCount));
  touch(3, 100); idle(50); CHECK(last == UiAction::ShowWeight); CHECK(!GrindBusy()); CHECK(lc.tares == tares);

  // --- Calibration through the menu ---
  writes = flash_store.writes;
  touch(0, 2100); touch(3, 100); CHECK(lastScreen == "Empty scale\r\nthen OK");
  touch(3, 100); idle(700); CHECK(lastScreen == "Put 100 g\r\n^v adj, OK");
  touch(1, 100); CHECK(lastScreen == "Put 200 g\r\n^v adj, OK");
  lc.rawCountsAboveTare = 696L * 200; touch(3, 100); idle(200); CHECK(lastScreen == "200.0 g\r\nOK to save");
  touch(3, 100); CHECK(lastScreen == "Saved"); CHECK(settings.calMassGrams == 200);
  CHECK(flash_store.writes == writes + 1); CHECK(settings.servoRestUs == 0);
  idle(200); CHECK(!GrindBusy());  // nor here

  // --- Servo setup: menu, ▼, OK; the watchdog runs while the arm is off rest ---
  touch(0, 2100); touch(2, 100); touch(3, 100);
  CHECK(lastScreen == "Rest  1024\r\n^v adj, OK"); CHECK(fakeServoUs == 1024); CHECK(!wdArmed);
  touch(1, 100); CHECK(lastScreen == "Rest  1034\r\n^v adj, OK"); CHECK(fakeServoUs == 1034);
  CHECK(fakeServoActive); CHECK(wdArmed);
  touch(1, 1000); CHECK(fakeServoUs >= 1034 + 10 * 6 && fakeServoUs <= 1034 + 10 * 8);
  int afterHold = fakeServoUs;
  touch(2, 100); touch(2, 100); CHECK(fakeServoUs == afterHold - 20);
  int restSet = fakeServoUs;
  touch(3, 100); CHECK(lastScreen == "Press 1292\r\n^v adj, OK"); CHECK(fakeServoUs == 1292); CHECK(wdArmed);
  touch(2, 100); touch(2, 100); CHECK(fakeServoUs == 1272);
  touch(3, 100); CHECK(lastScreen == "Saved"); CHECK(fakeServoUs == restSet); CHECK(!wdArmed);
  CHECK(settings.servoRestUs == restSet && settings.servoPressUs == 1272);
  CHECK(ServoRestUs() == restSet && ServoPressUs() == 1272);
  // Cancel mid-setup: back to the saved rest, nothing written
  writes = flash_store.writes;
  touch(0, 2100); touch(2, 100); touch(3, 100); touch(1, 100); CHECK(fakeServoUs == restSet + 10);
  touch(0, 100); CHECK(lastScreen == "Cancelled"); CHECK(fakeServoUs == restSet); CHECK(!wdArmed);
  CHECK(flash_store.writes == writes);
  idle(1000); CHECK(!GrindBusy()); CHECK(!fakeServoActive);  // letting go of ≡ didn't tare
  // Walk away while setting the press position: times out back to rest
  touch(0, 2100); touch(2, 100); touch(3, 100); touch(3, 100); CHECK(fakeServoUs == 1272);
  idle(19000); CHECK(fakeServoUs == 1272);
  idle(1500); CHECK(lastScreen == "Timed out"); CHECK(fakeServoUs == restSet); CHECK(!wdArmed);

  // --- A grind with the new positions ---
  CHECK(grindOnce() == 0);
  CHECK(GrindLastRecord().result == GrindResult::Done); CHECK(GrindLastRecord().targetG == 18.6f);

  // --- Cup taken away while settling: last steady reading, not learned ---
  float before = GrindOffsetG();
  rawG = 300; idle(300);
  touch(3, 100);
  CHECK(runUntil([] { return GrindStatus().s == "Settling..."; }, 30000));
  idle(800);
  float steady = reading();
  rawG -= 300;
  CHECK(runUntil([] { return !GrindBusy(); }, 500));
  CHECK(GrindLastRecord().result == GrindResult::Done); CHECK(!GrindLastRecord().learned);
  CHECK(fabsf(GrindLastRecord().actualG - steady) < 0.2f); CHECK(GrindOffsetG() == before);

  // --- Load cell missing at boot: OK refuses ---
  GrindBegin(false);
  tares = lc.tares;
  CHECK(status() == "Load cell error!");
  touch(3, 100); CHECK(!GrindBusy()); CHECK(lc.tares == tares); CHECK(lastBeepTimes == 3);
  GrindBegin(true);

  // --- A fresh boot picks up everything saved ---
  fakeServoUs = -1; ServoBegin(4); CHECK(fakeServoFirstUs == restSet);
  GrindBegin(true);
  CHECK(GrindDoseDg(1) == 93 && GrindDoseDg(2) == 186 && GrindSelectedDose() == 2);
  CHECK(fabsf(GrindOffsetG() - settings.offsetCg / 100.0f) < 0.001f);

  std::cout << "all grind / menu / settings checks passed (" << finished << " grinds, beeps=" << beeps << ")\n";
  return 0;
}
