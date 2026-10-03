#pragma once
#include <Arduino.h>

// How long a pad must be held to count as a long press
const unsigned long PAD_LONG_PRESS_MS = 2000;
// Auto-repeat while a pad is held: first repeat after the delay, then every
// PAD_REPEAT_MS
const unsigned long PAD_REPEAT_DELAY_MS = 500;
const unsigned long PAD_REPEAT_MS = 100;

// One TTP223 touch pad. Its output is HIGH while touched and the board cleans
// up the signal itself, so presses can be taken straight from digitalRead().
class Pad {
 public:
  explicit Pad(uint8_t pin) : pin_(pin) {}

  void begin() { pinMode(pin_, INPUT); }

  // Call once per loop, before checking the events below.
  void update() {
    unsigned long now = millis();
    bool down = (digitalRead(pin_) == HIGH);
    bool released = !down && down_;
    pressed_ = down && !down_;
    longPressed_ = false;
    repeated_ = false;
    if (pressed_) {
      downSince_ = now;
      longFired_ = false;
      repeated_ = true;
      nextRepeat_ = now + PAD_REPEAT_DELAY_MS;
    } else if (down && (long)(now - nextRepeat_) >= 0) {
      repeated_ = true;
      nextRepeat_ = now + PAD_REPEAT_MS;
    }
    if (down && !longFired_ && (now - downSince_ >= PAD_LONG_PRESS_MS)) {
      longPressed_ = true;
      longFired_ = true;
    }
    tapped_ = released && !longFired_;
    down_ = down;
  }

  bool pressed() const { return pressed_; }          // touched this loop
  bool repeated() const { return repeated_; }        // touched, or still held and due to repeat
  bool tapped() const { return tapped_; }            // let go before a long press
  bool longPressed() const { return longPressed_; }  // held PAD_LONG_PRESS_MS (once per hold)

  // The current touch has been used (e.g. to cancel), so letting go or
  // holding on must not also count as a tap or long press.
  void consume() { longFired_ = true; }

 private:
  uint8_t pin_;
  bool down_ = false;
  bool pressed_ = false;
  bool repeated_ = false;
  bool tapped_ = false;
  bool longPressed_ = false;
  bool longFired_ = false;
  unsigned long downSince_ = 0;
  unsigned long nextRepeat_ = 0;
};
