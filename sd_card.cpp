#include "sd_card.h"
#include "utils.h"
#include "time_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Data stored in RTC memory is not erased during deep sleep and only presit until reset.
// Hence it it better to use EEPROM or flash memory to store the data.

// use this variable to create unique picture name while saving in SD card
// this variable will be stored in the RTC memory (8Kb) and persist after deep sleep
// pay attention to the value because it can overflow. 
//RTC_DATA_ATTR uint8_t pictureNumberSaving;
//
//// use this variable to keep track of picture(s) that has been sent to phone
//RTC_DATA_ATTR uint8_t pictureNumberTransmitted = 0;

static SemaphoreHandle_t _sd_mmc_mutex = NULL;

/**
 * Initialize the SD card module.
 */
bool init_sd_card() {
 // start SD card and verify for the card.
 // setting, true disable the FLASH LED connected to GPIO4
  debug("init_sd_card: starting SD card");
  if(!SD_MMC.begin("/sdcard", true)){
    Serial.println("init_sd_card: sd card mount failed");
    return false;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("init_sd_card: no sd card attached");
    return false;
  }

  sd_total_space();
  sd_used_space();
  sd_free_space();
  
  // create the MUTEX for SD_MMC access. Errors when two processes uses SD_MMC at once.
  if(_sd_mmc_mutex == NULL){
    _sd_mmc_mutex = xSemaphoreCreateMutex();
    // xSemaphoreGive(_sd_mmc_mutex);
  }
  return true;
}

/**
 * Get SD_MMC and lock the mutex.
 */
void acquire_sd_mmc() {
  xSemaphoreTake(_sd_mmc_mutex, portMAX_DELAY);
}

/**
 * Release the SD_MMC mutex.
 */
void release_sd_mmc() {
  xSemaphoreGive(_sd_mmc_mutex);
}


uint32_t get_4_bytes_eeprom(uint8_t start_address) {
  uint32_t tp = 0;

  tp = EEPROM.read(start_address);
  tp |= EEPROM.read(start_address + 1) << 8;
  tp |= EEPROM.read(start_address + 2) << 16;
  tp |= EEPROM.read(start_address + 3) << 24;
  return tp;
}

void set_4_bytes_eeprom(uint8_t start_address, uint32_t value) {
  EEPROM.write(start_address, (uint8_t)(value & 0x000F));
  EEPROM.write(start_address, (uint8_t)(value & 0x00F0));
  EEPROM.write(start_address, (uint8_t)(value & 0x0F00));
  EEPROM.write(start_address, (uint8_t)(value & 0xF000));
  EEPROM.commit();
}

/**
 * Save the content of the camera buffer in the SD card.
 * @param: FS object
 * @param: Pointer to camera buffer structure.
 */
bool save_image_to_sd_card(fs::FS &fs, camera_fb_t * fb) {
  if (fb == NULL) {
    Serial.println("save_image_to_sd_card: null image buffer");
    return false;
  }
  
  // Path where new picture will be saved in SD Card
  show_current_rtc_time();
  String path = "/" + String(get_rtc_epoch_time(), 10) +".jpg";
  Serial.printf("save_image_to_sd_card: file name: %s\n", path.c_str());

  // get the file object to write the image data to SD card 
  File file = fs.open(path.c_str(), FILE_WRITE);
  
  if(!file){
    Serial.println("save_image_to_sd_card: failed to open file");
    return false;
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.println("save_image_to_sd_card: image saved");
  }

  // close the file
  file.close();
  return true;
}


/**
 * List the files and directory of the SD Card.
 * @param: FS object
 * @param: directory name (const char *)
 * @param: levels (uint8_t)
 * reference: https://github.com/espressif/arduino-esp32/blob/master/libraries/SD_MMC/examples/SDMMC_Test/SDMMC_Test.ino
 */
void sd_list_dir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                sd_list_dir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}


/**
 * Get the next file in the directory.
 * @param: FS object
 * @param: directory name (const char *)
 * @param: pointer to the FS File 
 * @return boolean
 */
bool sd_get_next_file(fs::FS &fs, const char * dirname, File * my_file){
  File root = fs.open(dirname);

  if(!root){
   debug("sd_get_next_file: failed to open directory");
    return false;
  }

  if(!root.isDirectory()){
    Serial.printf("sd_get_next_file: %s is not a directory\n", dirname);
    return false;
  }

  // open the next file in the directory
  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory()){
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
      // Serial.printf("sd_get_next_file: current file address 0x%x, current file value 0x%x\n", &file, file);
      *my_file = file;
      // root.close();
      return true;
    }
    file = root.openNextFile();
  }
}


/**
 * Read file from SD card and echo it to Serial monitor.
 * @param: FS
 * @param: file path (const char *)
 */
void sd_read_file(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
}

/**
 * Delete a file from the SD card.
 * @param: FS object
 * @param: file path (const char *)
 */
void sd_delete_file(fs::FS &fs, const char * path){
    Serial.printf("sd_delete_file: deleting file: %s\n", path);
    if(fs.remove(path)){
        debug("File deleted");
    } else {
        Serial.println("sd_delete_file: delete failed");
    }
}


/**
 * Get the used space of the sd card.
 * @return: uint64_t
 */
uint64_t get_sd_used_space(){
  return  SD_MMC.usedBytes() / (1024 * 1024);
}

/**
 * Print the used space of the SD card.
 */
void sd_used_space(){
  Serial.printf("SD used space: %lluMB\n", get_sd_used_space());
}

/**
 * Get total space of the sd card. 
 * @return uint64_t
 */
uint64_t get_sd_total_space(){
  return SD_MMC.totalBytes() / (1024 * 1024);
}

/** 
 * Print the total space of the SD Card. 
 */
void sd_total_space(){
  Serial.printf("SD total space: %lluMB\n", get_sd_total_space());
}

/**
 * Get free space of the sd card. 
 * @return uint64_t
 */
uint64_t get_sd_free_space(){
  return get_sd_total_space() - get_sd_used_space();
}

/**
 * Print the free space of the SD card.
 */
void sd_free_space(){
  Serial.printf("SD free space: %lluMB\n", get_sd_free_space());
}
