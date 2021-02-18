#ifndef __UTILS_H__
#define __UTILS_H__

#include "Arduino.h"
// #include "WiFi.h" 
// #include "driver/adc.h"
// #include <esp_wifi.h>
// #include <esp_bt.h>

#define DEBUG true
#define STATUS_OK 1
#define STATUS_NOT_OK 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */


/**
 * Send the string to serial port.
 * @param: Constant char pointer to the string.
 */
void debug(const char * message);

/**
 * Put's the ESP32 to deep sleep.
 */
void go_to_deep_sleep(long sleep_time_seconds);

#endif