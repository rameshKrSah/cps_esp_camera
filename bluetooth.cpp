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
    _bt_device_name = "cameraModule";
    _bt_connection_flag = BLUETOOTH_NONE;

    debug("Bluetooth: creating rcv data semaphore and mutex");
    if(_receive_data_Semaphore == NULL){
        _receive_data_Semaphore = xSemaphoreCreateBinary();
        xSemaphoreTake(_receive_data_Semaphore, 0);
    }

    if(_receive_data_mutex == NULL){
        _receive_data_mutex = xSemaphoreCreateMutex();
    }
    // xSemaphoreTake(_receive_data_mutex, 0);

    debug("Bluetooth: create the bluetooth serial mutex");
    if(_bluetooth_serial_mutex == NULL) {
        _bluetooth_serial_mutex = xSemaphoreCreateMutex();
    }
    // xSemaphoreTake(_bluetooth_serial_mutex, 0);
}

/**
 * Destructor for the Bluetooth class
 */
Bluetooth::~Bluetooth() {
    un_set_callbacks();
    _bt_serial.end();
    _bt_connection_flag = BLUETOOTH_DISCONNECTED;
    vSemaphoreDelete(_receive_data_Semaphore);
    _receive_data_Semaphore = NULL;

    vSemaphoreDelete(_receive_data_mutex);
    _receive_data_mutex = NULL;

    vSemaphoreDelete(_bluetooth_serial_mutex);
    _bluetooth_serial_mutex = NULL;
}

/**
 * Reset the Bluetooth module.
 */
void Bluetooth::de_init_bluetooth(){
    un_set_callbacks();
    _bt_serial.end();
    _bt_connection_flag = BLUETOOTH_DISCONNECTED;
}

/**
 * Initialize the Bluetooth module
 */
bool Bluetooth::init_bluetooth(uint8_t mac[6]) {

    if (!set_server_mac(mac)) {
        // raise an error
        Serial.println("init_bluetooth: no server MAC");
        return false;
    }

    // initialize the Bluetooth and set the callback
    _bt_serial.enableSSP();
    _bt_serial.begin(_bt_device_name, true);
    bool status = _bt_serial.connect(mac);

    // check if we are connected or not. if not try again.
   if(status) {
       Serial.println("init_bluetooth: connected");
       return true;
   }else {
       while(!_bt_serial.connected(1000)) {
           Serial.println("init_bluetooth: failed to connect"); 
       }
   }
    
    return true;
}

/**
 * Register the Bluetooth status callback.
 */
void Bluetooth::set_status_callback(esp_spp_cb_t * callback) {
    _bt_serial.register_callback(callback);
}

/**
 * Register the Bluetooth on data receive callback.
 */
void Bluetooth::set_on_receive_data_callback(BluetoothSerialDataCb callback){
    _bt_serial.onData(callback);
}

/**
 * Unregister the Bluetooth callbacks.
 */
void Bluetooth::un_set_callbacks() {
    _bt_serial.register_callback(NULL);
    _bt_serial.onData(NULL);
}

/**
 * Set server mac address.
 */
bool Bluetooth::set_server_mac(uint8_t mac[6]) {
    if (mac != NULL) {
        for(int i=0; i<6; ++i){
            _bt_server_mac[i] = mac[i];
        }
        return true;
    } 
    return false;
}

/**
 * Get Bluetooth device name.
 */
String Bluetooth::get_bt_device_name() {
    return _bt_device_name;
}

/**
 * Get the Bluetooth connection state. 
 */
_bluetooth_status_ Bluetooth::get_bt_connection_status() {
    return _bt_connection_flag;
}

/**
 * Set the Bluetooth connection flag.
 */
void Bluetooth::set_bt_connection_status(_bluetooth_status_ status) {
    Serial.printf("set_bt_connection_status: %s\n", _bluetooth_status_as_string(status));
    _bt_connection_flag = status;
}

/*
 * Send the data in the buffer to the output stream of Bluetooth
 */
int Bluetooth::bt_write_data(const uint8_t * buff, int len) {
  if((buff != NULL) && (len > 0)){
    // pass the data over to the output stream of Bluetooth
    debug("bt_write_data: sending ...");
    return _bt_serial.write(buff, len);
  }

  // if not return failed status
  Serial.println("bt_write_data: failed");
  return STATUS_NOT_OK;
}

/**
 * Reconnect to the server.
 */
void Bluetooth::bt_reconnect() {
    if (_bt_serial.hasClient() == 0) {
        // camera is not connected to any device
        debug("bt_reconnect: reconnecting");
        _bt_serial.connect();   // the same MAC address is used for reconnection
    }
}


/**
 * Get the Bluetooth connection status as string.
 */
const char * Bluetooth::_bluetooth_status_as_string(_bluetooth_status_ st) {
    switch(st) {
        case BLUETOOTH_NONE:
            return "BLUETOOTH_NONE";
        
        case BLUETOOTH_CONNECTING: 
            return "BLUETOOTH_CONNECTING";

        case BLUETOOTH_CONNECTED:
            return "BLUETOOTH_CONNECTED";

        case BLUETOOTH_DISCONNECTED:
            return "BLUETOOTH_DISCONNECTED";
        
        default:
            return "UNKNOWN";
    }
}

bool Bluetooth::take_rcv_data_semaphore() {
    if(xSemaphoreTake(_receive_data_Semaphore, ( TickType_t ) 1000) == pdTRUE){
        return true;
    }

    Serial.println("take_rcv_data_semaphore: failed");
    return false;
}

/**
 * Release the receive data mutex.
 */
void Bluetooth::give_rcv_data_mutex(){
    // debug("give_rcv_data_mutex");
    xSemaphoreGive(_receive_data_mutex);
}

/**
 * Copy the data received from the Bluetooth in the receive buffer.
 * @param: const uint8_t * buff
 * @param: uint8_t len
 */
void Bluetooth::copy_received_data(const uint8_t * buff, uint16_t len) {
    // we may need mutex to protect the receive data buffer to be free before writing into it.
    xSemaphoreTake(_receive_data_mutex, ( TickType_t ) 50);
    memcpy(_read_buffer, buff, len);
    _receive_length = len;
    xSemaphoreGive(_receive_data_mutex);

    // give the Semaphore for any process waiting on the receive data.
    xSemaphoreGive(_receive_data_Semaphore);
    // debug("copy_received_data: mutex released, and semaphore given");
}

/**
 * Get the first byte from the receive buffer.
 * @return uint8_t
 */
uint8_t * Bluetooth::get_recv_buffer() {
    xSemaphoreTake(_receive_data_mutex, ( TickType_t ) 5);
    return _read_buffer;
}

/**
 * Get the received data length on Bluetooth.
 * @return uint16_t
 */
uint16_t Bluetooth::get_recv_buffer_length() {
    return _receive_length;
}

/**
 * Take the Bluetooth serial mutex. We need this for sequential operation of the Bluetooth.
 * When one operation is going on with the Bluetooth, another operation must wait untils it finishes.
 */
void Bluetooth::take_bluetooth_serial_mutex() {
    xSemaphoreTake(_bluetooth_serial_mutex, portMAX_DELAY);
}

/**
 * Release the Bluetooth serial mutex.
 */
void Bluetooth::release_bluetooth_serial_mutex() {
    xSemaphoreGive(_bluetooth_serial_mutex);
}