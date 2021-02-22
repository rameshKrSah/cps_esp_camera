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

/**
 * List the files and directory of the SD Card.
 * @param: FS object
 * @param: directory name (const char *)
 * @param: levels (uint8_t)
 * reference: https://github.com/espressif/arduino-esp32/blob/master/libraries/SD_MMC/examples/SDMMC_Test/SDMMC_Test.ino
 */
void sd_list_dir(fs::FS &fs, const char * dirname, uint8_t levels);


/**
 * Read file from SD card and echo it to Serial monitor.
 * @param: FS
 * @param: file path (const char *)
 */
void sd_read_file(fs::FS &fs, const char * path);

/**
 * Delete a file from the SD card.
 * @param: FS object
 * @param: file path (const char *)
 */
void sd_delete_file(fs::FS &fs, const char * path);

/**
 * Get the used space of the sd card.
 * @return: uint32_t
 */
uint32_t get_sd_used_space();

/**
 * Print the used space of the SD card.
 */
void sd_used_space();

/**
 * Get total space of the sd card. 
 * @return uint32_t
 */
uint32_t get_sd_total_space();

/** 
 * Print the total space of the SD Card. 
 */
void sd_total_space();

/**
 * Get free space of the sd card. 
 * @return uint32_t
 */
uint32_t get_sd_free_space();

/**
 * Print the free space of the SD card.
 */
void sd_free_space();

/**
 * Get the next file in the directory.
 * @param: FS object
 * @param: directory name (const char *)
 * @param: pointer to the FS:File 
 * @return boolean
 */
bool sd_get_next_file(fs::FS &fs, const char * dirname, File * my_file);

#endif
