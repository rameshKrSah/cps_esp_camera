#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include "Arduino.h"
#include <ESP32Time.h>
#include <stdint.h>
#include "time.h"
#include <sys/time.h>
#include "utils.h"

/**
 * Print current RTC time to Serial.
 */
void show_current_rtc_time();

/**
 * Get current RTC time as string.
 * @param: char * to store the time string.
 */
void get_rtc_time_as_string(char * buffer);

/**
 * Set the RTC time using the epoch time.
 * @param: uint64_t seconds elapsed time
 */
void set_rtc_time(uint64_t epoch_time);

/**
 * Get current RTC epoch time as a string.
 * @param: const char * of length 50
 */
void get_rtc_epoch_time_as_string(char * buffer);

#endif