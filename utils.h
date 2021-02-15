#ifndef __UTILS_H__
#define __UTILS_H__

#include "Arduino.h"

#define DEBUG true
#define STATUS_OK 1
#define STATUS_NOT_OK -1

/**
 * Send the string to serial port.
 * @param: Constant char pointer to the string.
 */
void debug(const char * message);

#endif