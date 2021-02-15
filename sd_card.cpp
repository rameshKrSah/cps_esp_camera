#include "sd_card.h"
#include "utils.h"

// use this variable to create unique picture name while saving in SD card
// int pictureNumberSaving;

// // use this variable to keep track of picture(s) that has been sent to phone
// int pictureNumberTransmitted;


/**
 * Initialize the SD card module.
 */
boolean init_sd_card() {
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


/*
 * Save the content of the camera buffer in the SD card.
 * @param: Pointer to camera buffer structure.
 */
boolean save_image_to_sd_card(camera_fb_t * fb, int pictureNumberSaving) {
    if (fb == NULL) {
        return false;
    }
    
    // Path where new picture will be saved in SD Card
    String path = "/picture" + String(pictureNumberSaving) +".jpg";
    debug("new picture file name: ");
    debug(path.c_str());

    // get the file object to write the image data to SD card
    fs::FS &fs = SD_MMC; 
    File file = fs.open(path.c_str(), FILE_WRITE);
    
    if(!file){
    //   debug("failed to open file in writing mode");
      return false;
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      debug("saved image to path: ");
      debug(path.c_str());
    }

    // close the file
    file.close();
    return true;
}
