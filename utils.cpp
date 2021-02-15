#include "utils.h"

/*
 * This is our debug function that spits out messages and errors 
 * to the serial monitor
 */
void debug(const char * message){
  if (DEBUG) {
    Serial.println(message);
  }
}
