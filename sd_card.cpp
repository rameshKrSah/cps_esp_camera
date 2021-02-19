#include "sd_card.h"
#include "utils.h"

// Data stored in RTC memory is not erased during deep sleep and only presit until reset.
// Hence it it better to use EEPROM or flash memory to store the data.

// use this variable to create unique picture name while saving in SD card
// this variable will be stored in the RTC memory (8Kb) and persist after deep sleep
// pay attention to the value because it can overflow. 
//RTC_DATA_ATTR uint8_t pictureNumberSaving;
//
//// use this variable to keep track of picture(s) that has been sent to phone
//RTC_DATA_ATTR uint8_t pictureNumberTransmitted = 0;


/**
 * Initialize the SD card module.
 */
bool init_sd_card() {
 // start SD card and verify for the card.
  debug("starting SD card");
  if(!SD_MMC.begin()){
    debug("sd card mount failed");
    return false;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    debug("no sd card attached");
    return false;
  }
  
  return true;
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

/*
 * Save the content of the camera buffer in the SD card.
 * @param: Pointer to camera buffer structure.
 */
bool save_image_to_sd_card(camera_fb_t * fb) {
    if (fb == NULL) {
        return false;
    }
    
    uint32_t picture_number;
        
    // get the last picture number from EEPROM, and increment it for the new picture
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, picture_number);
    picture_number += 1;
    
    // Path where new picture will be saved in SD Card
    String path = "/picture" + String(picture_number) +".jpg";
    debug("new picture file name: ");
    debug(path.c_str());

    // get the file object to write the image data to SD card
    fs::FS &fs = SD_MMC; 
    File file = fs.open(path.c_str(), FILE_WRITE);
    
    if(!file){
      debug("failed to open file in writing mode");
      return false;
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      debug("saved image to path: ");
      debug(path.c_str());

      // save the updated picture number into EEPROM
      EEPROM.put(0, picture_number);
      EEPROM.commit();
    }

    // close the file
    file.close();
    return true;
}
