#pragma once
#include <FlashStorage.h>
#include <FlashSettings.h>

// Reserve a portion of flash memory to store a settings and call it "flash_store"
extern FlashStorageClass<FlashSettings> flash_store;
extern FlashSettings settings;
