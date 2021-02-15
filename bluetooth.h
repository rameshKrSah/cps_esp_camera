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

class Bluetooth {
    private:
    BluetoothSerial bt_serial;
    String bt_device_name;
    uint8_t bt_server_mac[6];
    bool bt_connection_flag;
    // void callbackBluetooth(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    // void bluetoothOnDataReceiveCallback(const uint8_t * buff, size_t len);

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
    bool get_bt_connection_status();

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
    void set_bt_connection_status(bool status);
};


#endif
