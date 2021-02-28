#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include "Arduino.h"
#include <ESP32Time.h>
#include <stdint.h>

/**
 * Get RTC time as string in the format "%d-%m-%Y:%H-%M-%S"
 */
void get_rtc_time_as_string();

/**
 * Set the RTC time using the epoch time.
 * @param: uint64_t seconds elapsed time
 */
void set_rtc_time(uint64_t epoch_time);


#endif