#pragma once
#include "Arduino.h"
#include "FlashSettings.h"
struct FakeStore { int writes = 0; FlashSettings saved{}; void write(const FlashSettings& s) { writes++; saved = s; } };
extern FakeStore flash_store; extern FlashSettings settings;
void SaveSettings();
