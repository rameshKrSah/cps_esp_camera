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
 
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "utils.h"
#include "camera.h"
#include "sd_card.h"
#include "bluetooth.h"

#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

Bluetooth my_bluetooth;
uint8_t btServerAddress[6] = {0xC4, 0x50, 0x06, 0x83, 0xF4, 0x7E};
xSemaphoreHandle deep_sleep_semaphore;

/**
 * We will have two tasks. 
 * 1. One task to take pictures and save the pictures to SD card. 
 * 2. Second task to connect to phone via Bluetooth and transmit images in the SD card.
 * 
 * To schedule a task, we need a function that contains the code we want to run and then 
 * create a task that calls this function. 
 */

/**
 * Task to take pictures and save the pictures in the SD card
 */
void camera_task(void * params) {
  debug("camera task started!");

  // strucutre that holds the camera data
  camera_fb_t * fb = NULL;
  fb = take_picture();
  Serial.printf("camera buf len %d\n", fb->len);
  
  // if we have a picture, try to store it in the SD card.
  if(!save_image_to_sd_card(fb)) {
    debug("failed to save image to card");
  }

  // return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  // Turn off the on board LED
  turn_off_camera_flash();

  for(;;) {
    debug("going to deep sleep from camera task");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // wait for the semaphore for deep sleep
    xSemaphoreTake(deep_sleep_semaphore, portMAX_DELAY);

    // go to deep sleep
    go_to_deep_sleep(TIME_TO_SLEEP);
  }
}


/**
 * Task to connect to phone via Bluetooth and send pictures stored in the SD card.
 */
void bluetooth_task(void * params) {
  debug("bluetooth task started!");

  for(;;) {
    // check whether we have data in the SD card or not

    // check whether we have connection or not
    if(my_bluetooth.get_bt_connection_status() != BLUETOOTH_CONNECTED) {
      my_bluetooth.bt_reconnect();
    }

    // send the data untill done or transmit fixed number of images.

    // my_bluetooth.de_init_bluetooth();
    
    // give the Semaphore so that the camera can be put to sleep.

    xSemaphoreGive(deep_sleep_semaphore);

    // delete the task.
    debug("deleting bluetooth task");
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
  }
}


// Setup Part
void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  // Start the serial communication
  if(DEBUG) {
    Serial.begin(115200);
  }
  debug("configuring services");

  // initialize the Bluetooth
 if(!my_bluetooth.init_bluetooth(btServerAddress))
 {
   debug("bluetooth init failed");
   return;
 }

 // register Bluetooth callbacks.
 my_bluetooth.set_status_callback(bt_status_callback);
 my_bluetooth.set_on_receive_data_callback(bt_data_received_callback);
  
  // initialize the camera module
   if (init_camera() != ESP_OK) {
    debug("camera init failed");
    return;
  }
  
  // Turn off the on board LED
  turn_off_camera_flash();

  // initialize the SD module
  if(!init_sd_card()) {
    debug("sd card init failed");
    return; 
  }

  // create the deep sleep semaphore
  vSemaphoreCreateBinary(deep_sleep_semaphore);
  // xSemaphoreGive(deep_sleep_semaphore);

  // Schedule the tasks. 
  xTaskCreate(camera_task, "take pictue and save to sd card", 5000, NULL, 1, NULL);
  xTaskCreate(bluetooth_task, "connect to phone and send data", 5000, NULL, 1, NULL);
}

// Loop part of the code. 
void loop() {
  // // check whether the Bluetooth Connection is Intact or not.
  // if(my_bluetooth.get_bt_connection_status() != BLUETOOTH_CONNECTED) {
  //  my_bluetooth.bt_reconnect();
  // }
  
//   // strucutre that holds the camera data
//   camera_fb_t * fb = NULL;
//   fb = take_picture();
  
//   // send the read image to the Phone via Bluetooth
//   Serial.printf("camera buf len %d\n", fb->len);
// //  my_bluetooth.bt_write_data(fb->buf, fb->len);
  
//   // if we have a picture, try to store it in the SD card.
//  if(!save_image_to_sd_card(fb)) {
//     debug("failed to save image to card");
//   }

//   // return the frame buffer back to the driver for reuse
//   esp_camera_fb_return(fb);

//   // Turn off the on board LED
//   turn_off_camera_flash();

  // before going to sleep stop the bluetooth
  // my_bluetooth.de_init_bluetooth();
  // go_to_deep_sleep(TIME_TO_SLEEP);
}


/*
 * Callback which receives the data from the input stream of Bluetooth
 */
void bt_data_received_callback(const uint8_t * buff, size_t len) {
    uint8_t totalBytes = int(len);
    if((buff != NULL) && (totalBytes > 0)){
      Serial.printf("got data on Bluetooth length: %d\n", totalBytes);
      my_bluetooth.copy_received_data(buff, totalBytes);
    }
}

/*
 * Callback function for the Bluetooth stack. We can monitor the Bluetooth status from 
 * here.
 */
void bt_status_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  debug(getSPP_CB_EventDes(event));
  
  switch(event) {
    case ESP_SPP_OPEN_EVT:  // camera is connected to the phone
      debug("cb: camera connected to phone");
      my_bluetooth.set_bt_connection_status(BLUETOOTH_CONNECTED);
      break;

    case ESP_SPP_CLOSE_EVT: // camera is disconnected from the phone
      debug("cb: camera disconnected from phone");
      my_bluetooth.set_bt_connection_status(BLUETOOTH_DISCONNECTED);

    //   Serial.printf("cb: %d\n", param->close.async ? 1 : 0);

      // do we reconnect or what
      break;
      
    case ESP_SPP_START_EVT: // camera as the Bluetooth server
      debug("cb: camera as server");
      break;
      
    case ESP_SPP_CL_INIT_EVT: // camera started the connection to the phone
      debug("cb: camera connecting to phone");
      my_bluetooth.set_bt_connection_status(BLUETOOTH_CONNECTING);
      break;
      
    case ESP_SPP_SRV_OPEN_EVT: // camera as server has accepted an incoming connection
      debug("cb: camera as server has a connection");
      break;
      
   case ESP_SPP_SRV_STOP_EVT: // camera as server has lost a connection
     debug("cb: camera as server has lost a connection");
     break;
      
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
 * Get text description of the SPP callback event.
 */
const char * getSPP_CB_EventDes(esp_spp_cb_event_t st) {
  switch(st){
    case ESP_SPP_INIT_EVT:
      return "ESP_SPP_INIT_EVT: SPP is inited";
    
   case ESP_SPP_UNINIT_EVT:
     return "ESP_SPP_UNINIT_EVT: SPP is uninited";
      
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
      
   case ESP_SPP_SRV_STOP_EVT:
     return "ESP_SPP_SRV_STOP_EVT: SPP server stopped";

    default:
      return "invalid";
    
  }
}

/*
 * Get text description of the SPP status enum.
 */
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
