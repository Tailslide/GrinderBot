/**
 ui.cpp - touch pad routing and the settings menu
**/

#include <Arduino.h>
#include "ui.h"
#include "panelcal.h"
#include "servosetup.h"
#include "valueedit.h"
#include "grind.h"
#include "net.h"
#include "display.h"
#include "FlashStore.h"

namespace {

enum MenuItem { ITEM_CALIBRATE, ITEM_SERVO, ITEM_DOSE1, ITEM_DOSE2, ITEM_OFFSET, ITEM_NETWORK, MENU_COUNT };
const char* const MENU_ITEMS[MENU_COUNT] = {"Calibrate", "Servo pos", "Dose 1", "Dose 2", "Offset",
                                            "Network"};

const unsigned long INFO_REFRESH_MS = 1000;
const unsigned long INFO_TIMEOUT_MS = 30000;

bool menuOpen = false;
int menuIndex = 0;
bool infoOpen = false;
unsigned long infoOpened = 0;
unsigned long infoDrawn = 0;

const unsigned long OK_CONFIRM_MS = 150;  // a grind starts this long after OK is let go
bool okSpoiled = false;                   // this OK touch overlapped another pad
bool okPending = false;                   // OK tapped, grind about to start
unsigned long okReleasedAt = 0;

// Two items per screen; '>' marks the selection
void showMenu(Adafruit_SSD1306& display) {
  int first = (menuIndex / 2) * 2;
  String text;
  for (int i = first; i < first + 2 && i < MENU_COUNT; i++) {
    if (i > first) text = text + "\r\n";
    text = text + (i == menuIndex ? ">" : " ") + MENU_ITEMS[i];
  }
  DisplayMessage(display, text);
}

void showInfo(Adafruit_SSD1306& display) {
  String text = NetInfo();
  text = text + "\nGrinds " + String((int)settings.grindCount);
  DisplayLines(display, text);
  infoDrawn = millis();
}

void saveDose1(int tenths) { GrindSetDoseDg(1, tenths); }
void saveDose2(int tenths) { GrindSetDoseDg(2, tenths); }
void saveOffset(int tenths) {
  // OK without changing it keeps the learned value (finer than 0.1 g)
  if (tenths != (int)lroundf(GrindOffsetG() * 10.0f)) GrindSetOffsetG(tenths / 10.0f);
}

void editDose(Adafruit_SSD1306& display, int n) {
  ValueEditStart(display, n == 1 ? "Dose 1" : "Dose 2", GrindDoseDg(n), DOSE_MIN_DG, DOSE_MAX_DG,
                 n == 1 ? saveDose1 : saveDose2);
}

void openItem(HX711_ADC& LoadCell, Adafruit_SSD1306& display, int item) {
  switch (item) {
    case ITEM_CALIBRATE: PanelCalStart(LoadCell, display); break;
    case ITEM_SERVO: ServoSetupStart(display); break;
    case ITEM_DOSE1: editDose(display, 1); break;
    case ITEM_DOSE2: editDose(display, 2); break;
    case ITEM_OFFSET:
      ValueEditStart(display, "Offset", (int)lroundf(GrindOffsetG() * 10.0f), 0,
                     (int)lroundf(OFFSET_MAX_G * 10.0f), saveOffset);
      break;
    case ITEM_NETWORK:
      infoOpen = true;
      infoOpened = millis();
      showInfo(display);
      break;
  }
}

}  // namespace

bool UiIdle() {
  return !menuOpen && !infoOpen && !PanelCalActive() && !ServoSetupActive() && !ValueEditActive();
}

UiAction UiUpdate(HX711_ADC& LoadCell, Adafruit_SSD1306& display,
                  Pad& menuPad, Pad& upPad, Pad& downPad, Pad& okPad, bool tareDone) {
  bool menuTouch = menuPad.pressed();
  bool anyTouch = menuTouch || upPad.pressed() || downPad.pressed() || okPad.pressed();

  // OK sits next to 2 v, and a finger on one pad can be picked up by its
  // neighbour too. An OK touch that overlaps a touch on another pad is
  // ignored: it can't select, save or start a grind.
  bool othersDown = menuPad.down() || upPad.down() || downPad.down();
  if (okPad.pressed()) okSpoiled = othersDown;
  else if (okPad.down() && othersDown) okSpoiled = true;
  bool okPress = okPad.pressed() && !othersDown;
  if (GrindBusy() || !UiIdle() || othersDown) okPending = false;

  // A grind in progress: any touch stops it. Touches are used up, so letting
  // go doesn't also start something. While settling or zeroing, touches are ignored.
  Pad* pads[4] = {&menuPad, &upPad, &downPad, &okPad};
  if (GrindBusy()) {
    if (anyTouch) {
      for (Pad* p : pads) p->consume(true);
      if (GrindRunning()) {
        GrindStop();
        beep();
      }
    }
    return UiAction::ShowWeight;
  }

  // In a menu or setting, every touch belongs to that screen: letting go
  // after the screen has closed (OK to save, ≡ to cancel) mustn't count as a
  // tap on the weight screen, where OK starts a grind and ≡ tares.
  // (Auto-repeat still works, for holding ▲/▼.)
  if (!UiIdle()) {
    for (Pad* p : pads) {
      if (p->pressed()) p->consume();
    }
  }

  if (PanelCalActive()) {
    PanelCalUpdate(LoadCell, display, menuTouch, upPad.repeated(), downPad.repeated(),
                   okPress, tareDone);
    return UiAction::None;
  }
  if (ServoSetupActive()) {
    ServoSetupUpdate(display, menuTouch, upPad.repeated(), downPad.repeated(), okPress);
    return UiAction::None;
  }
  if (ValueEditActive()) {
    ValueEditUpdate(display, menuTouch, upPad.repeated(), downPad.repeated(), okPress);
    return UiAction::None;
  }
  if (infoOpen) {
    if (anyTouch || millis() - infoOpened >= INFO_TIMEOUT_MS) {
      infoOpen = false;
      beep();
      return UiAction::ShowWeight;
    }
    if (millis() - infoDrawn >= INFO_REFRESH_MS) showInfo(display);
    return UiAction::None;
  }
  if (menuOpen) {
    if (menuTouch) {
      menuOpen = false;  // back to the weight screen
      beep();
      return UiAction::ShowWeight;
    } else if (upPad.pressed()) {
      menuIndex = (menuIndex + MENU_COUNT - 1) % MENU_COUNT;
      showMenu(display);
    } else if (downPad.pressed()) {
      menuIndex = (menuIndex + 1) % MENU_COUNT;
      showMenu(display);
    } else if (okPress) {
      menuOpen = false;
      beep();
      openItem(LoadCell, display, menuIndex);
    }
    return UiAction::None;
  }

  // Weight screen
  if (menuPad.longPressed()) {
    beep();
    menuOpen = true;
    menuIndex = 0;
    showMenu(display);
    return UiAction::None;
  }
  if (menuPad.tapped()) {
    beep();
    GrindZero(LoadCell);
  }
  Pad* dosePads[3] = {nullptr, &upPad, &downPad};
  for (int n = 1; n <= 2; n++) {
    if (dosePads[n]->longPressed()) {
      // Still being held: don't let it auto-repeat into the editor
      dosePads[n]->consume(true);
      beep();
      GrindSelectDose(n);
      editDose(display, n);
      return UiAction::None;
    }
    if (dosePads[n]->tapped()) {
      beep();
      GrindSelectDose(n);
    }
  }
  // OK starts the grind once it has been let go for OK_CONFIRM_MS with no
  // other pad touched, so a finger brushing OK on its way to 2 v doesn't
  // start one either.
  if (okPad.tapped() && !okSpoiled) {
    okPending = true;
    okReleasedAt = millis();
  }
  if (okPending && millis() - okReleasedAt >= OK_CONFIRM_MS) {
    okPending = false;
    GrindStart(LoadCell);
  }
  return UiAction::ShowWeight;
}
