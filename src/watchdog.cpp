/**
 watchdog.cpp - SAMD21 watchdog, armed while the servo holds the grinder button
**/

#include <Arduino.h>
#include "watchdog.h"

namespace {

bool armed = false;
bool clockReady = false;

void waitSync() {
  while (WDT->STATUS.bit.SYNCBUSY) {}
}

// Watchdog clock: generic clock generator 2 from the internal 32.768 kHz
// ultra-low-power oscillator, divided by 32 (2^(4+1)) = 1.024 kHz.
// The Arduino core reserves generator 2 for the watchdog.
void setupClock() {
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);
  GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_GENEN |
                      GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_DIVSEL;
  while (GCLK->STATUS.bit.SYNCBUSY) {}
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2;
  clockReady = true;
}

}  // namespace

void WatchdogArm() {
  if (armed) return;
  if (!clockReady) setupClock();
  WDT->CTRL.reg = 0;  // must be disabled while configuring
  waitSync();
  WDT->INTENCLR.bit.EW = 1;          // no early-warning interrupt
  WDT->CONFIG.bit.PER = 0x8;         // 2048 clock cycles = 2 s
  WDT->CTRL.bit.WEN = 0;             // normal (not window) mode
  waitSync();
  WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
  waitSync();
  WDT->CTRL.bit.ENABLE = 1;
  waitSync();
  armed = true;
}

void WatchdogDisarm() {
  if (!armed) return;
  WDT->CTRL.bit.ENABLE = 0;
  waitSync();
  armed = false;
}

void WatchdogFeed() {
  // Skip if the previous clear is still crossing into the slow clock domain;
  // the loop comes round again in a few ms, well inside the 2 s period.
  if (armed && !WDT->STATUS.bit.SYNCBUSY) WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
}

bool WatchdogCausedLastReset() {
  return (PM->RCAUSE.reg & PM_RCAUSE_WDT) != 0;  // (.bit.WDT collides with the WDT macro)
}
