#pragma once
#include <Arduino.h>
#include "grind.h"

// Wi-Fi + MQTT logging to Home Assistant (optional).
//
// Only built in when include/secrets.h exists (copy secrets.example.h and fill
// it in). Without it, or with Wi-Fi down, the scale works exactly the same and
// these calls do nothing.
//
// Each finished grind is published as one retained JSON message on
// grinderbot/<id>/grind, and Home Assistant MQTT discovery sets up a
// "GrinderBot" device with sensors for the last dose, target, grind time,
// offset, result and a grind count. If the broker is down, the latest grind
// is kept and sent on reconnect.

void NetBegin();
// Call every loop.
//   canPoll:  no grind running and the servo at rest. Quick MQTT traffic only
//             (keep-alive, publishing a finished grind).
//   canBlock: also no menu open. Connecting to Wi-Fi or the broker can block
//             for a few seconds, so it only happens then.
void NetUpdate(bool canPoll, bool canBlock);
void NetPublishGrind(const GrindRecord& record);
bool NetConnected();  // MQTT connected
String NetInfo();     // three short lines ("\n" between) for the Network screen
