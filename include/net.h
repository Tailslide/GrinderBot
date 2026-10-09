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
// grinderbot/<id>/grind (and on grinderbot/<id>/dose if it reached the
// target), and Home Assistant MQTT discovery sets up a "GrinderBot" device
// with sensors for the last dose, target, grind time, offset, result and a
// grind count. If the broker is down, the latest grind is kept and sent on
// reconnect.

void NetBegin();
// Call every loop. Nothing here runs while a grind is going or the servo is
// off rest. Calls can block (the long watchdog period covers a real hang):
//   canPoll:  no grind running and the servo at rest. MQTT keep-alive and
//             publishing a finished grind: normally quick, but a dying link
//             can stall each write for ~2.5 s, or ~5 s to close the socket.
//   canBlock: also no menu open. Connecting to Wi-Fi or the broker, ~11 s
//             at worst when the broker is unreachable, so only then.
void NetUpdate(bool canPoll, bool canBlock);
void NetPublishGrind(const GrindRecord& record);
bool NetConnected();  // MQTT connected
String NetInfo();     // three short lines ("\n" between) for the Network screen
