#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include <EEPROM.h>            // read and write from flash memory
#include "esp_camera.h"

// define the number of bytes you want to access
#define EEPROM_SIZE 4

/**
 * Initialize the SD card module.
 */
bool init_sd_card();


/*
 * Save the content of the camera buffer in the SD card.
 * @param: Pointer to camera buffer structure.
 */
bool save_image_to_sd_card(camera_fb_t * fb);

#endif
