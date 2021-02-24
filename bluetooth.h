#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "Arduino.h"

#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include <algorithm>
#include <iterator>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum {
    BLUETOOTH_NONE = 0,
    BLUETOOTH_CONNECTING = 1,
    BLUETOOTH_CONNECTED = 2,
    BLUETOOTH_DISCONNECTED = 3
}_bluetooth_status_;

// maximum size of send packet is 1024
const uint16_t MAX_LENGTH = 1024;

class Bluetooth {
    private:
    BluetoothSerial _bt_serial;
    String _bt_device_name;
    uint8_t _bt_server_mac[6];
    _bluetooth_status_ _bt_connection_flag;
    uint8_t _read_buffer[MAX_LENGTH];
    uint8_t _receive_length;

    SemaphoreHandle_t _receive_data_Semaphore = NULL;
    SemaphoreHandle_t _receive_data_mutex = NULL;

    const char * _bluetooth_status_as_string(_bluetooth_status_ st);

    public:
    Bluetooth();
    ~Bluetooth();

    /**
     * Initialize the Bluetooth module
     */
    bool init_bluetooth(uint8_t mac[6]);

    /**
     * Set Bluetooth server MAC address.
     */
    bool set_server_mac(uint8_t mac[6]);

    /**
     * Get the Bluetooth device name for the camera.
     */
    String get_bt_device_name();

    /**
     * Get the Bluetooth connection status.
     */
    _bluetooth_status_ get_bt_connection_status();

    /*
    * Send the data in the buffer to the output stream of Bluetooth
    */
    int bt_write_data(const uint8_t * buff, int len);
    
    /**
     * Register the Bluetooth status callback.
     */
    void set_status_callback(esp_spp_cb_t * callback);

    /**
     * Register the Bluetooth on data receive callback.
     */
    void set_on_receive_data_callback(BluetoothSerialDataCb callback);

    /**
     * Unregister the Bluetooth callbacks.
     */
    void un_set_callbacks();

    /**
     * Reconnect to the server.
     */
    void bt_reconnect();

    /**
    Reset the Bluetooth module.
    */
    void de_init_bluetooth();

    /**
     * Set the Bluetooth connection flag.
     */
    void set_bt_connection_status(_bluetooth_status_ status);

    /**
     * Take the receive data semaphore.
     */
    bool take_rcv_data_semaphore();

    /**
     * Copy the data received from the Bluetooth in the receive buffer.
     * @param: const uint8_t * buff
     * @param: uint8_t len
     */
    void copy_received_data(const uint8_t * buff, uint8_t len);

    /**
     * Get the first byte from the receive buffer.
     * @return uint8_t
     */
    uint8_t get_recv_buffer();
};


#endif
