// Pulls the real sketch into a host build: the SD card is simulated
// (stubs/SD.h), the RTC is left out.
#pragma once
#include "Arduino.h"
#include "../../../firmware/SCC_EnvControl/config.h"
#undef USE_RTC
#include "../../../firmware/SCC_EnvControl/SCC_EnvControl.ino"
