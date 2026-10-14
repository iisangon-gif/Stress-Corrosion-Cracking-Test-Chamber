// Pulls the real sketch into a host build (no SD card / RTC on the PC).
#pragma once
#include "Arduino.h"
#include "../../../firmware/SCC_EnvControl/config.h"
#undef USE_SD
#undef USE_RTC
#include "../../../firmware/SCC_EnvControl/SCC_EnvControl.ino"
