#include <FlashStorage.h>
#include "FlashSettings.h"

// Reserve a portion of flash memory to store a settings and call it "flash_store"
FlashStorage(flash_store, FlashSettings);

FlashSettings settings = flash_store.read();
