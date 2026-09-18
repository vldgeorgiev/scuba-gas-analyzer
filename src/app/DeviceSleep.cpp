#include "DeviceSleep.h"

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
namespace app {
RTC_DATA_ATTR uint32_t retainedSleepMarker = 0;
}
#endif