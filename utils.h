#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stddef.h>

#include "Arduino.h"
// #include "WiFi.h" 
// #include "driver/adc.h"
// #include <esp_wifi.h>
// #include <esp_bt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define DEBUG false
#define STATUS_OK 1
#define STATUS_NOT_OK 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */


// void debug(const char * format, ...)  __attribute__ ((format (printf, 2, 3)));
// #define debug(fmt, ...) _debug(fmt"\n", ##__VA_ARGS__)
// size_t printf(const char * format, ...)  __attribute__ ((format (printf, 2, 3)));

/**
 * Send the string to serial port.
 * @param: Constant char pointer to the string.
 */
void debug(const char * message);

/**
 * Put's the ESP32 to deep sleep.
 */
void go_to_deep_sleep(long sleep_time_seconds);

/**
 * Delay function, takes milliseconds argument for delay and calss VTaskDelay
 */
void delay_ms(uint32_t ms);

#endif