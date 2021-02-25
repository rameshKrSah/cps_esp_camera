#ifndef __TIME_MANAGER__
#define __TIME_MANAGER__

#include "Arduino.h"

/**
 * Get RTC time as string in the format "%d-%m-%Y:%H-%M-%S"
 */
String get_rtc_time_as_string();

/**
 * Set the RTC time using the epoch time.
 * @param: uint64_t seconds elapsed time
 */
void set_rtc_time(uint64_t epoch_time);


#endif