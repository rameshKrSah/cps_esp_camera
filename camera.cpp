#include "camera.h"
#include "utils.h"

/**
 * Initialize the camera module.
 */
esp_err_t init_camera() {
  // configure the camera module
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 

  // set the frame size and picture quality
  if(psramFound()){
    debug("frame size: UXGA, quality: 12");
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    debug("frame size: SVGA, quality: 12");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  debug("starting camera");
  return esp_camera_init(&config);
}


/**
 * Take a picture and return the pointer to camera buffer.
 */
camera_fb_t * take_picture() {
  // pointer to camera frame buffer object
  camera_fb_t * fb = NULL;

  // get the pointer to the camera frame buffer which has the content of latest photo
  fb = esp_camera_fb_get();  
  if(!fb) {
    debug("camera capture failed, trying again");
    fb = esp_camera_fb_get();
    if(!fb) {
      debug("camera capture failed again");
      return NULL;
    }
  }
  return fb;
}


/**
 * Turn off the camer flash.
 */
void turn_off_camera_flash(){
    pinMode(4, INPUT);
    digitalWrite(4, LOW);
    rtc_gpio_hold_dis(GPIO_NUM_4);
}

