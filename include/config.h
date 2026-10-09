#pragma once
// Settings specific to one GrinderBot: the load cell, the servo, the grinder
// and your doses.
//
// These are the defaults used when nothing has been saved yet, which is the
// case after every firmware upload (it wipes the saved settings). Once running,
// doses, offsets, servo positions, max grind time and calibration can all be
// changed from the menu.
//
// To use your own values without changing this file, copy
// include/config_local.example.h to include/config_local.h (git-ignored) and
// define only the ones you want to change there. Anything defined in
// config_local.h, or with -D in platformio.ini build_flags, wins.

#if defined(__has_include) && !defined(GRINDERBOT_NO_LOCAL_CONFIG)
#if __has_include("config_local.h")
#include "config_local.h"
#endif
#endif

// --- Load cell ---
// Raw counts per gram. Panel calibration prints the factor over serial when it
// saves; put it here so readings stay in grams after an upload.
#ifndef DEFAULT_CAL_FACTOR
#define DEFAULT_CAL_FACTOR 1635.7321f
#endif

// --- Servo (pulse widths in microseconds) ---
// Menu -> Servo pos shows the current values ("Rest 1234", then OK, "Press 1234").
#ifndef SERVO_DEFAULT_REST_US
#define SERVO_DEFAULT_REST_US 1024   // arm clear of the manual button
#endif
#ifndef SERVO_DEFAULT_PRESS_US
#define SERVO_DEFAULT_PRESS_US 1292  // arm holding the manual button down
#endif

// --- Doses, in tenths of a gram (167 = 16.7 g) ---
#ifndef DOSE1_DEFAULT_DG
#define DOSE1_DEFAULT_DG 167
#endif
#ifndef DOSE2_DEFAULT_DG
#define DOSE2_DEFAULT_DG 153
#endif

// --- Grinder ---
// Never hold the button longer than this (seconds). Set from Menu -> Max time.
#ifndef GRIND_MAX_DEFAULT_S
#define GRIND_MAX_DEFAULT_S 120
#endif
// Starting offset for each dose: how far short of the target to let go. It's
// learned per dose after that.
#ifndef OFFSET_DEFAULT_G
#define OFFSET_DEFAULT_G 1.0f
#endif
// Let go ("No flow: hopper?") if the weight hasn't risen 0.3 g in this long.
// Raise it if a slow grinder trips it at the start of a grind.
#ifndef GRIND_STALL_MS
#define GRIND_STALL_MS 5000UL
#endif
