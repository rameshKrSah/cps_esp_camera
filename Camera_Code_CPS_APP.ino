/*
 * ESP-32 Camera Module Code for the CPS Human in the loop project. 
 * The camera module has the following functions
 * 1. Take images periodically and store them in the SD card
 * 2. Communicate with the phone (Android app) using Bluetooth. 
 * 3. Transmit images saved in the SD card to the phone over Bluetooth.
 * 4. Send and receive other information to/from phone over Bluetooth.
 * 5. Remain in the deep sleep mode unless work is to be done.
 * 6. Maintain a RTC for time keeping. Query time over internet maybe at each reset.
 */

/*
 * First part of the project is to take images periodically (deep sleep maintained) 
 * and store them in SD card.
 */


#include "utils.h"
#include "camera.h"
#include "sd_card.h"

#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
//#include <SoftwareSerial.h> 

// // define the number of bytes you want to access
// #define EEPROM_SIZE 2

// // Pin definition for CAMERA_MODEL_AI_THINKER
// #define PWDN_GPIO_NUM     32
// #define RESET_GPIO_NUM    -1
// #define XCLK_GPIO_NUM      0
// #define SIOD_GPIO_NUM     26
// #define SIOC_GPIO_NUM     27

// #define Y9_GPIO_NUM       35
// #define Y8_GPIO_NUM       34
// #define Y7_GPIO_NUM       39
// #define Y6_GPIO_NUM       36
// #define Y5_GPIO_NUM       21
// #define Y4_GPIO_NUM       19
// #define Y3_GPIO_NUM       18
// #define Y2_GPIO_NUM        5
// #define VSYNC_GPIO_NUM    25
// #define HREF_GPIO_NUM     23
// #define PCLK_GPIO_NUM     22

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

/*
 * store the picture number in the RTC DATA section to 
 * maintain this value between resets
 */
// use this variable to create unique picture name while saving in SD card
// RTC_DATA_ATTR int pictureNumberSaving = 0;

// // use this variable to keep track of picture(s) that has been sent to phone
// RTC_DATA_ATTR int pictureNumberTransmitted = 0;

// Whether to print debug over serial or not
//#define DEBUG true
// #define STATUS_OK 1
// #define STATUS_NOT_OK -1


// what are these for
// File photoFile; 
// int global_i;


/*
 * TODO:
 * 1. we need to implement to check when the connection is established, 
 * 2. data is transmitted successfully
 * 3. do we have some data in the receive queue
 * 4. if connection is lost do we reconnect and finish the job
 * 5. .........
 */

#include "BluetoothSerial.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
//#include <esp_log.h>

BluetoothSerial SerialBT;

const String btDeviceName = "cameraModule"; 
String MACadd = "C4:50:06:83:F4:7E";
uint8_t btServerAddress[6]  = {0xC4, 0x50, 0x06, 0x83, 0xF4, 0x7E};
String btServerName = "SAMSUMG-S4";
bool btConnectionFlag = false;

/*
 * This is our debug function that spits out messages and errors 
 * to the serial monitor
 */
//void debug(const char * message){
//  if (DEBUG) {
//    Serial.println(message);
//  }
//}

/*
 * When the ESP32-CAM takes a photo, it flashes the on-board LED. 
 * After taking the photo, the LED remains on, so we send 
 * instructions to turn it off. The LED is connected to GPIO 4.
 */
// void manageOnBoardLED() {  
//   pinMode(4, INPUT);
//   digitalWrite(4, LOW);
//   rtc_gpio_hold_dis(GPIO_NUM_4);
// }

/*
 * Initialize the camera with proper setting
 */
// esp_err_t setup_camera() {
//   // configure the camera module
//   camera_config_t config;
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer = LEDC_TIMER_0;
//   config.pin_d0 = Y2_GPIO_NUM;
//   config.pin_d1 = Y3_GPIO_NUM;
//   config.pin_d2 = Y4_GPIO_NUM;
//   config.pin_d3 = Y5_GPIO_NUM;
//   config.pin_d4 = Y6_GPIO_NUM;
//   config.pin_d5 = Y7_GPIO_NUM;
//   config.pin_d6 = Y8_GPIO_NUM;
//   config.pin_d7 = Y9_GPIO_NUM;
//   config.pin_xclk = XCLK_GPIO_NUM;
//   config.pin_pclk = PCLK_GPIO_NUM;
//   config.pin_vsync = VSYNC_GPIO_NUM;
//   config.pin_href = HREF_GPIO_NUM;
//   config.pin_sscb_sda = SIOD_GPIO_NUM;
//   config.pin_sscb_scl = SIOC_GPIO_NUM;
//   config.pin_pwdn = PWDN_GPIO_NUM;
//   config.pin_reset = RESET_GPIO_NUM;
//   config.xclk_freq_hz = 20000000;
//   config.pixel_format = PIXFORMAT_JPEG; 

//   // set the frame size and picture quality
//   if(psramFound()){
//     debug("frame size: UXGA, quality: 12");
//     config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
//     config.jpeg_quality = 10;
//     config.fb_count = 2;
//   } else {
//     debug("frame size: SVGA, quality: 12");
//     config.frame_size = FRAMESIZE_SVGA;
//     config.jpeg_quality = 12;
//     config.fb_count = 1;
//   }
  
//   debug("starting camera");
//   return esp_camera_init(&config);
// }

/*
 * Initialize the SD card module
 */
// boolean setup_sd_card(){
//  // start SD card and verify for the card.
//   debug("starting SD card");
//   if(!SD_MMC.begin()){
//     debug("sd card mount failed");
//     return false;
//   }
  
//   uint8_t cardType = SD_MMC.cardType();
//   if(cardType == CARD_NONE){
//     debug("no sd card attached");
//     return false;
//   } 

//   return true;
// }

/*
 * Return the pointer to camera buffer with content of latest photo. 
 * The structure also has a timeStamp variable. Can we use this to infer 
 * the time at which the photo was taken.
 */
// camera_fb_t * takePicture() {
//   // pointer to camera frame buffer object
//   camera_fb_t * fb = NULL;

//   // get the pointer to the camera frame buffer which has the content of latest photo
//   fb = esp_camera_fb_get();  
//   if(!fb) {
//     debug("camera capture failed, trying again");
//     fb = esp_camera_fb_get();
//     if(!fb) {
//       debug("camera capture failed again");
//       return NULL;
//     }
//   }
//   return fb;
// }

/*
 * Save the content of the camera buffer in the SD card.
 */
// boolean saveToCard(camera_fb_t * fb) {
//     // Path where new picture will be saved in SD Card
//     String path = "/picture" + String(pictureNumberSaving) +".jpg";
//     debug("new picture file name: ");
//     debug(path.c_str());

//     // get the file object to write the image data to SD card
//     fs::FS &fs = SD_MMC; 
//     File file = fs.open(path.c_str(), FILE_WRITE);
    
//     if(!file){
//       debug("failed to open file in writing mode");
//       return false;
//     }
//     else {
//       file.write(fb->buf, fb->len); // payload (image), payload length
//       debug("saved image to path: ");
//       debug(path.c_str());
//     }

//     // close the file
//     file.close();
//     return true;
// }

/*
 * Get text description of the SPP status enum.
 *
const char * getSPP_StatusDes(esp_spp_status_t st) {
  switch(st){
    case ESP_SPP_SUCCESS:
      return "ESP_SPP_SUCCESS: Successful operation";
      
    case ESP_SPP_FAILURE:
      return "ESP_SPP_FAILURE: Generic failure";
      
    case ESP_SPP_BUSY:
      return "ESP_SPP_BUSY: Temporarily can not handle this request";
      
    case ESP_SPP_NO_DATA:
      return "ESP_SPP_NO_DATA: no data";
      
    case ESP_SPP_NO_RESOURCE:
      return "ESP_SPP_NO_RESOURCE: No more resource";
      
    case ESP_SPP_NEED_INIT:
      return "ESP_SPP_NEED_INIT: SPP module shall init first";
      
    case ESP_SPP_NEED_DEINIT:
      return "ESP_SPP_NEED_DEINIT: SPP module shall deinit first";
      
    case ESP_SPP_NO_CONNECTION:
      return "ESP_SPP_NO_CONNECTION: connection may have been closed";

    default:
      return "invalid";
  }
}
*/

/*
 * Get text description of the SPP callback event.
 */
const char * getSPP_CB_EventDes(esp_spp_cb_event_t st) {
  switch(st){
    case ESP_SPP_INIT_EVT:
      return "ESP_SPP_INIT_EVT: SPP is inited";
    
//    case ESP_SPP_UNINIT_EVT:
//      return "ESP_SPP_UNINIT_EVT: SPP is uninited";
      
    case ESP_SPP_DISCOVERY_COMP_EVT:
      return "ESP_SPP_DISCOVERY_COMP_EVT: SDP discovery complete";
      
    case ESP_SPP_OPEN_EVT:
      return "ESP_SPP_OPEN_EVT: SPP Client connection open";
      
    case ESP_SPP_CLOSE_EVT:
      return "ESP_SPP_CLOSE_EVT: SPP connection closed";
      
    case ESP_SPP_START_EVT:
      return "ESP_SPP_START_EVT: SPP server started";
      
    case ESP_SPP_CL_INIT_EVT:
      return "ESP_SPP_CL_INIT_EVT: SPP client initiated a connection";
      
    case ESP_SPP_DATA_IND_EVT:
      return "ESP_SPP_DATA_IND_EVT: SPP connection received data";
      
    case ESP_SPP_CONG_EVT:
      return "ESP_SPP_CONG_EVT: SPP connection congestion status changed";
      
    case ESP_SPP_WRITE_EVT:
      return "ESP_SPP_WRITE_EVT: SPP write operation completes";
      
    case ESP_SPP_SRV_OPEN_EVT:
      return "ESP_SPP_SRV_OPEN_EVT: SPP Server connection open";
      
//    case ESP_SPP_SRV_STOP_EVT:
//      return "ESP_SPP_SRV_STOP_EVT: SPP server stopped";

    default:
      return "invalid";
    
  }
}

/*
 * Callback function for the Bluetooth stack. We can monitor the Bluetooth status from 
 * here.
 */
void callbackBluetooth(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  debug(getSPP_CB_EventDes(event));
  
  switch(event) {
    case ESP_SPP_OPEN_EVT:  // camera is connected to the phone
      debug("cb: camera connected to phone");
      break;

    case ESP_SPP_CLOSE_EVT: // camera is disconnected from the phone
      debug("cb: camera disconnected from phone");
      Serial.printf("cb: %d\n", param->close.async ? 1 : 0);
      // do we reconnect or what
      if (!SerialBT.hasClient()) {
        // camera is not connected to any device
        SerialBT.connect();   // the same MAC address is used for reconnection
      }
      break;
      
    case ESP_SPP_START_EVT: // camera as the Bluetooth server
      debug("cb: camera as server");
      break;
      
    case ESP_SPP_CL_INIT_EVT: // camera started the connection to the phone
      debug("cb: camera connecting to phone");
      break;
      
    case ESP_SPP_SRV_OPEN_EVT: // camera as server has accepted an incoming connection
      debug("cb: camera as server has a connection");
      break;
      
//    case ESP_SPP_SRV_STOP_EVT: // camera as server has lost a connection
//      debug("cb: camera as server has lost a connection");
//      break;
      
    case ESP_SPP_DATA_IND_EVT: // data is received over Bluetooth
      debug("cb: camera has received data on BT");
      Serial.printf("# bytes received %d\n", param->data_ind.len);
      break;
      
    case ESP_SPP_WRITE_EVT: // data write over Bluetooth is completed
      debug("cb: camera has written data on BT");
      Serial.printf("# bytes written %d\n", param->write.len);
      break;
      
    case ESP_SPP_CONG_EVT: // congestion status of Bluetooth has changed
      debug("cb: camera BT congestion status changed");
      break;

    default:
      debug("cb: other event");
      break;
  }
}

/*
 * Callback which receives the data from the input stream of Bluetooth
 */
void bluetoothOnDataReceiveCallback(const uint8_t * buff, size_t len) {
  int totalBytes = int(len);
  if((buff != NULL) && (totalBytes>0)){
    // we got data on the input stream of Bluetooth
    // do something with it. Don't process it here. Use Semaphore maybe
    
  }
}

/*
 * Send the data in the buffer to the output stream of Bluetooth
 */
int bluetoothWriteData(const uint8_t * buff, int len) {
  if((buff != NULL) && (len > 0)){
    // pass the data over to the output stream of Bluetooth
    return SerialBT.write(buff, len);
  }

  // if not return failed status
  return STATUS_NOT_OK;
}

/*
 * Setup Bluetooth and connect to the server. 
 */
boolean setupBluetooth() {
  SerialBT.begin(btDeviceName, true);
  SerialBT.register_callback(callbackBluetooth);
  btConnectionFlag = SerialBT.connect(btServerAddress);
  if(btConnectionFlag && SerialBT.hasClient()) {
    debug("bt connected to phone");
    return true;
  }else {
    while(!SerialBT.connected(1000)) {
      debug("camera failed to connect. make sure phone is available and in range"); 
    }
  }

  // connection failed deregister the callback
  SerialBT.register_callback(NULL);
  return false;
}


void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  // Start the serial communication
  if(DEBUG) {
    Serial.begin(115200);
  }
  debug("configuring services");

  // Turn off the on board LED
  turn_off_camera_flash();
  
  // initialize the Bluetooth
  if(!setupBluetooth())
  {
    debug("bluetooth init failed");
    return;
  }
  
  // initialize the camera module
   if (init_camera() != ESP_OK) {
    debug("camera init failed");
    return;
  }

  // initialize the SD module
  if(!init_sd_card()) {
    debug("sd card init failed");
    return; 
  }

  
}


void loop() {
  // check whether the Bluetooth Connection is Intact or not.
  Serial.printf("# clients %d\n", SerialBT.hasClient());
  if(!SerialBT.hasClient()){
    btConnectionFlag = SerialBT.connect(btServerAddress);
    if(btConnectionFlag && SerialBT.hasClient()) {
      debug("bt connected to phone");
    }else {
      // connection failed deregister the callback
      SerialBT.register_callback(NULL);
    }
  }
  
  // strucutre that holds the camera data
  camera_fb_t * fb = take_picture();
  
  // Turn off the on board LED
  turn_off_camera_flash();

  
  // send the read image to the Phone via Bluetooth
  Serial.printf("camera buf len %d\n", fb->len);
  bluetoothWriteData(fb->buf, fb->len);

  
//  // if we have a picture, try to store it in the SD card.
// if(!save_image_to_sd_card(fb)) {
//    debug("failed to save image to card");
//  }

  // return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);


  // read the data from the SD card 

  
//  uint8_t dt[5] = {'h', 'e', 'l', 'l', 'o'};
//  
//  SerialBT.write(dt, 5);
//  
//  if (SerialBT.available()) {
//    Serial.write(SerialBT.read());
//  }

  delay(10000);

  
//  
//  // going to deep_sleep, based on the sleep configuration. 
//  // For us it will timer based wake up. 
//  // enable wake-up using the RTC time, uses time in micro seconds
//  if (esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR) == ESP_OK) {
//    debug("going to sleep");
//    // go to deep sleep
//    esp_deep_sleep_start();
//  }
}



  
//  file = fs.open(path);
//  if(!file){
//    Serial.println("Failed to open file for reading");
//    return;
//  }
//  
//  int jpglen = fb->len;
//  while (jpglen > 0) {
//    // read 32 bytes at a time;
//    uint8_t *buffer;
//    uint8_t bytesToRead = min(32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
//    //      buffer = cam.readPicture(bytesToRead);
//    file.write(buffer, bytesToRead);
//    //file.write(fb->buf, fb->len);
//    for(i=0; i<bytesToRead;i++)
//    SerialBT.write(buffer[i]);
//           
//    jpglen -= bytesToRead;
//    }
//  
//  //Serial.print("Read from file: ");
//  // while(SerialBT.available()){
//  //SerialBT.write(file.read());
//  delay(2000);
//  //}
