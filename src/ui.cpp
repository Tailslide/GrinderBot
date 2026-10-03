/**
 ui.cpp - touch pad routing and the settings menu
**/

#include <Arduino.h>
#include "ui.h"
#include "panelcal.h"
#include "servosetup.h"
#include "display.h"

namespace {

const char* const MENU_ITEMS[] = {"Calibrate", "Servo pos"};
const int MENU_COUNT = 2;

bool menuOpen = false;
int menuIndex = 0;

// Both items fit on the two-line screen; '>' marks the selection
void showMenu(Adafruit_SSD1306& display) {
  String text;
  for (int i = 0; i < MENU_COUNT; i++) {
    text = text + (i == menuIndex ? ">" : " ") + MENU_ITEMS[i];
    if (i < MENU_COUNT - 1) text = text + "\r\n";
  }
  DisplayMessage(display, text);
}

}  // namespace

UiAction UiUpdate(HX711_ADC& LoadCell, Adafruit_SSD1306& display,
                  Pad& menuPad, Pad& upPad, Pad& downPad, Pad& okPad, bool tareDone) {
  // Inside the menu or a setting, a ≡ touch means back/cancel, so letting go
  // of it mustn't also count as a tap (which would run the servo test).
  bool menuTouch = menuPad.pressed();
  bool busy = menuOpen || PanelCalActive() || ServoSetupActive();
  if (busy && menuTouch) menuPad.consume();

  if (PanelCalActive()) {
    PanelCalUpdate(LoadCell, display, menuTouch, upPad.repeated(), downPad.repeated(),
                   okPad.pressed(), tareDone);
    return UiAction::None;
  }
  if (ServoSetupActive()) {
    ServoSetupUpdate(display, menuTouch, upPad.repeated(), downPad.repeated(), okPad.pressed());
    return UiAction::None;
  }
  if (menuOpen) {
    if (menuTouch) {
      menuOpen = false;  // back to the weight screen
      beep();
    } else if (upPad.pressed()) {
      menuIndex = (menuIndex + MENU_COUNT - 1) % MENU_COUNT;
      showMenu(display);
    } else if (downPad.pressed()) {
      menuIndex = (menuIndex + 1) % MENU_COUNT;
      showMenu(display);
    } else if (okPad.pressed()) {
      menuOpen = false;
      beep();
      if (menuIndex == 0) PanelCalStart(LoadCell, display);
      else ServoSetupStart(display);
    }
    return UiAction::None;
  }

  if (menuPad.longPressed()) {
    beep();
    menuOpen = true;
    menuIndex = 0;
    showMenu(display);
    return UiAction::None;
  }
  if (menuPad.tapped()) return UiAction::ServoTest;
  return UiAction::ShowWeight;
}
