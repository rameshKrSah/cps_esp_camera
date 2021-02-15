#include "bluetooth.h"
#include "utils.h"


// const String btDeviceName = "cameraModule"; 
// String MACadd = "C4:50:06:83:F4:7E";
// uint8_t btServerAddress[6]  = {0xC4, 0x50, 0x06, 0x83, 0xF4, 0x7E};
// String btServerName = "SAMSUMG-S4";
// bool btConnectionFlag = false;

/**
 * Constructor for the Bluetooth class. 
 * @param: uint8_t pointer of the server MAC address.
 */
Bluetooth::Bluetooth() {
    bt_device_name = "cameraModule";
}

/**
 * Destructor for the Bluetooth class
 */
Bluetooth::~Bluetooth() {
    un_set_callbacks();
    bt_serial.end();
    bt_connection_flag = false;
}

/**
 * Reset the Bluetooth module.
 */
void Bluetooth::de_init_bluetooth(){
    un_set_callbacks();
    bt_serial.end();
    bt_connection_flag = false;
}

/**
 * Initialize the Bluetooth module
 */
bool Bluetooth::init_bluetooth(uint8_t mac[6]) {

    if (!set_server_mac(mac)) {
        // raise an error
        debug("Bluetooth needs server MAC address to work.");
        return false;
    }

    // initialize the Bluetooth and set the callback
    bt_serial.begin(bt_device_name, true);
    bt_connection_flag = bt_serial.connect(bt_server_mac);

    // check if we are connected or not. if not try again.
    if(bt_connection_flag && bt_serial.hasClient()) {
        debug("bt connected to phone");
        return true;
    }else {
        while(!bt_serial.connected(1000)) {
            debug("camera failed to connect. make sure phone is available and in range"); 
        }
    }

    return false;
}

/**
 * Register the Bluetooth status callback.
 */
void Bluetooth::set_status_callback(esp_spp_cb_t * callback) {
     bt_serial.register_callback(callback);
}

/**
 * Register the Bluetooth on data receive callback.
 */
void Bluetooth::set_on_receive_data_callback(BluetoothSerialDataCb callback){
    bt_serial.onData(callback);
}

/**
 * Unregister the Bluetooth callbacks.
 */
void Bluetooth::un_set_callbacks() {
    bt_serial.register_callback(NULL);
    bt_serial.onData(NULL);
}

/**
 * Set server mac address.
 */
bool Bluetooth::set_server_mac(uint8_t mac[6]) {
    if (mac != NULL) {
        for(int i=0; i<6; ++i){
            bt_server_mac[i] = mac[i];
        }
        return true;
    } 
    return false;
}

/**
 * Get Bluetooth device name.
 */
String Bluetooth::get_bt_device_name() {
    return bt_device_name;
}

/**
 * Get the Bluetooth connection state. 
 */
bool Bluetooth::get_bt_connection_status() {
    return bt_connection_flag;
}

/**
 * Set the Bluetooth connection flag.
 */
void Bluetooth::set_bt_connection_status(bool status) {
    bt_connection_flag = status;
}

/*
 * Send the data in the buffer to the output stream of Bluetooth
 */
int Bluetooth::bt_write_data(const uint8_t * buff, int len) {
  if((buff != NULL) && (len > 0)){
    // pass the data over to the output stream of Bluetooth
    return bt_serial.write(buff, len);
  }

  // if not return failed status
  return STATUS_NOT_OK;
}

/**
 * Reconnect to the server.
 */
void Bluetooth::bt_reconnect() {
    if (bt_serial.hasClient() == 0) {
        // camera is not connected to any device
        bt_connection_flag = bt_serial.connect();   // the same MAC address is used for reconnection
    }
}


