#pragma once

// Buzzer helpers (defined in main.cpp; non-blocking, via tone())
void beep();             // one short beep
void beepTimes(int n);   // n short beeps: 2 = grind done, 3 = grind stopped by a safety check
