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

/**
 * Put's the ESP32 to deep sleep.
 */
void go_to_deep_sleep(long sleep_time_seconds) {
  debug("going to deep sleep");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  adc_power_off();
  esp_wifi_stop();

  // configure the timer to wake up
  esp_sleep_enable_timer_wakeup(sleep_time_seconds * uS_TO_S_FACTOR);

  // go to sleep 
  esp_deep_sleep_start();
}
