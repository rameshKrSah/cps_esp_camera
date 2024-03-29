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

// void debug(const char * format, ...) {
//     va_list arg;

//     /* Write the error message */
//     va_start(arg, format);
//     Serial.printf(format, arg);
//     va_end(arg);
// }


/**
 * Put's the ESP32 to deep sleep.
 */
void go_to_deep_sleep(long sleep_time_seconds) {
  Serial.println("go_to_deep_sleep");
  
  // configure the timer to wake up
  esp_sleep_enable_timer_wakeup(sleep_time_seconds * uS_TO_S_FACTOR);

  // go to sleep 
  esp_deep_sleep_start();
}

/**
 * Delay function, takes milliseconds argument for delay and calss VTaskDelay
 */
void delay_ms(uint32_t ms){
  vTaskDelay(ms / portTICK_PERIOD_MS); 
}
