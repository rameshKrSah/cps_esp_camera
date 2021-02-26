/**
 * Handles the Bluetooth communication between the camera and phone.
 * 
 * Procedure for sending data to the phone.
 * 
 * * 1. Check if there is a Bluetooth connection.
 * * 2. If yes, create the data packet else return.
 * * 3. Send the data packet. 
 * * 4. Wait for the response from the phone. 
 * * 5. Repeat if necessary
 * 
 * Packet Details::
 * We can send upto 330 bytes of data at one time over Bluetooth.
 * 
 * * The first byte of the packet will indicate the data type. Possible 
 * values are 1) Image (0xA0), 2) General (0xB0). 
 * 
 * * The second byte is the payload length. 
 * 
 * * If the data type is image type the third byte will indicate the packet 
 * number since image data needs to be divided before transmission. 
 * 
 * The payload starts from 3rd-byte position or 4th-byte position (if image) 
 * data. 
 * 
 * The last byte will an ending character indicating the end of current
 * transmission. We set it to '#'.
 * 
 * Phone and camera needs to send a response packet after reception of data. 
 * The response will contain the first byte, 1 for the second byte, thrid
 * byte if any, received data length as payload, and the ending character. 
 * 
 * Both phone and camera will wait for 5 seconds for response. 
 */

#include "bluetooth_comm.h"
#include "utils.h"
#include "sd_card.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Constructor for the BluetoothCommuninication Class
BluetoothCommunication::BluetoothCommunication(){
    if(_data_written_semaphore == NULL){
        _data_written_semaphore = xSemaphoreCreateBinary();
        if(_data_written_semaphore == NULL) {
            debug("BluetoothCommunication: failed to create _data_written_semaphore");
        }
        xSemaphoreTake(_data_written_semaphore, 0);
    }
}

// Destructor for the BluetoothCommuninication Class
BluetoothCommunication::~BluetoothCommunication(){}


/**
 * Send next image from the SD card to phone.
 * 
 * @param: Bluetooth object pointer
 * @param: FS object
 */
bool BluetoothCommunication::send_next_image(Bluetooth * my_bt, fs::FS &fs){
    bool status = false;

    File root_dir = fs.open("/");
    // check whether root is open and a directory
    if(root_dir == NULL || !root_dir.isDirectory()) { 
        debug("send_next_image: failed to open the SD card");    
        return status;
    }

    // open the next file in the directory
    File my_file = root_dir.openNextFile();
    while(my_file){
        if(!my_file.isDirectory()){
            debug("send_next_image:  FILE: ");
            Serial.print(my_file.name());
            debug("send_next_image:  SIZE: ");
            Serial.println(my_file.size());
            break;
        }
        my_file = root_dir.openNextFile();
    }

    // send the file 
    status = send_data_file(my_bt, BT_DATA, &my_file);

    // close the file
    my_file.close();
    root_dir.close();

    // after the file is sent, delete it from SD card.

    return status;
}

/**
 * Send the content of the file over Bluetooth. 
 * @param: Bluetooth object
 * @param: Bluetooth communication type
 * @param: FILE object pointer
 * 
 * @return: boolean
 */
bool BluetoothCommunication::send_data_file(Bluetooth * my_bt, _bluetooth_comm_type comm_type, File * my_file) {
    bool status = false;

    // check the file pointer
    if(my_file == NULL) {
        debug("send_data_file: null file pointer");
        return status;
    }

    uint32_t file_size = my_file->size();
    Serial.printf("send_data_file: file size %d\n", file_size);

    // First we need to check if we have the connection
    if(my_bt->get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        debug("send_data_file: bt disconnected");
        return status;
    }

    // set the packet number and allocate memory for the data packet (MAX_LENGTH) bytes
    _packet_number = 1;

    // _temp_buffer_buffer = (uint8_t *) malloc(empty_packet_space * sizeof(uint8_t));
    uint16_t read_size = 0;
    uint32_t total_bytes_sent = 0;

    while(my_file->available()){
        // read bytes from the file
        read_size = my_file->read(_packet_buffer + _PREAMBLE_SIZE, _PAYLOAD_SPACE);

        Serial.printf("size %d, position %d, packet number %d\n", my_file->size(), my_file->position(), _packet_number);
        Serial.printf("read %d bytes from file\n", read_size);

        // send the read bytes to phone
        status = _send_data(my_bt, comm_type, NULL, read_size, false);
        if(status) {
            total_bytes_sent += read_size;
        }
        _packet_number += 1;
    }

    // free the allocated memory
    // if (_temp_buffer_buffer != NULL) {
    //     debug("freeing the allocated memory");
    //     // free the allocated memory
    //     free(_temp_buffer_buffer);
    // }

    Serial.printf("send_data_file: out of %d bytes, %d sent\n", file_size, total_bytes_sent);
    return status;
}

/**
 * Send the data over Bluetooth by creating a data packet and
 * receive the response.
 * 
 * @param Bluetooth object pointer
 * @param Bluetooth communication type
 * @param uint8_t * pointer to the data array
 * @param uint16_t data length
 * 
 * @return boolean.
 */
bool BluetoothCommunication::send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, 
    const uint8_t * data_ptr, uint16_t data_length) {

    bool status = false;
    uint16_t total_data_length = data_length;

    // First we need to check if we have the connection
    if(my_bt->get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        debug("send_data: bt disconnected");
        return status;
    }

    // set the packet number to zero
    _packet_number = 1;

    // check the length, send the data, and receive response
    if (data_length > _PAYLOAD_SPACE) {
        uint16_t start = 0;
        uint16_t end = _PAYLOAD_SPACE;
        uint16_t sending_bytes = 0;

        // we need to divide data into several packets.
        while(total_data_length != 0){
            // keep sending until we reach the end of the data buffer
            Serial.printf("sending packet number %d\n", _packet_number);
            sending_bytes = end - start;
            Serial.printf("start: %d, end: %d, # bytes %d\n", start, end, sending_bytes);

            // copy the data into the buffer
            memcpy(_packet_buffer + _PREAMBLE_SIZE, data_ptr + start, sending_bytes);
            status = _send_data(my_bt, comm_type, NULL, sending_bytes, true);

            if(status) {
                total_data_length -= sending_bytes;
                start += sending_bytes;
                end += total_data_length < _PAYLOAD_SPACE ? total_data_length : _PAYLOAD_SPACE;
                _packet_number += 1;
            } else {
                // failed to send data, break free.
               break;
            }
        }

        Serial.printf("send_data: out of %d bytes, %d sent\n", data_length, end);
    } else {
        // copy the data into the buffer
        memcpy(_packet_buffer + _PREAMBLE_SIZE, data_ptr, data_length);
        status = _send_data(my_bt, comm_type, NULL, data_length, true);
    }

    return status;
}

/**
 * Send data over Bluetooth. All other send functions calls this function to send data. 
 * 
 * @param: Bluetooth object pointer (must use pointer otherwise the )
 * @param: Bluetooth communication type
 * @param: uint8_t * pointer to payload
 * @param: uint16_t payload length
 * @param: bool whether to wait for response or not.
 */
bool BluetoothCommunication::_send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, 
    const uint8_t * data_ptr, uint16_t data_length, bool response) {

    bool status = false;
    uint8_t tx_failed = 0;
    
    // create the data packet
    _create_packet(comm_type, data_ptr, data_length);

    while(true){
        // we have the data packet. send it over Bluetooth and wait for the response
        if(my_bt->bt_write_data(_packet_buffer, _packet_length) == _packet_length) {
            debug("_send_data: data sent succesully");
            status = true;

            // Do we wait for the response?
            if(response) {
                debug("_send_data: waiting for response");
                status = _wait_for_response(my_bt, comm_type);
            }
            break;
        } else {
            debug("_send_data: failed to send data");
            tx_failed++;
        }

        // try 3 times
        if(tx_failed == 3) {
            // don't need to wait for the semaphore
            return false;
        }
        
    }

    // wait for the semaphore
    debug("_send_data: waiting for data written semaphore");
    if(xSemaphoreTake(_data_written_semaphore, portMAX_DELAY) != pdTRUE){
        debug("_send_data: failed to obtain data written semaphore");
    }
    return status;
}

/**
 * Wait for the response from the phone on Bluetooth and verfies the response.
 * @param: Bluetooth communication type
 * @return: boolean
 */
bool BluetoothCommunication::_wait_for_response(Bluetooth * my_bt, _bluetooth_comm_type comm_type) {
    // wait for the Semaphore for 50 ticks 
    if(my_bt->take_rcv_data_semaphore()) {
        debug("_wait_for_response: response received from phone");
        // if we reach here, then we have received response and it is in the receive buffer.
        if(my_bt->get_recv_buffer() == comm_type){
            // data was sent succesully
            debug("_wait_for_response: data type matched");
            return true;
        }
    }

    // data transmission Failed
    return false;
}

/**
 * Create the packet to be sent over Bluetooth.
 * @param: Bluetooth communication type
 * @param: uint8_t * pointer to payload
 * @param: uint16_t payload_len
 */
void BluetoothCommunication::_create_packet(_bluetooth_comm_type comm_type, const uint8_t * payload, 
        uint16_t payload_len){

    _packet_length = 0;
    // uint8_t * packet_bf_ptr = _packet_buffer;
    // Serial.printf("dt ptr 0x%X, length %d\n", packet_bf_ptr, _packet_length);

    // is this request or data
    *(_packet_buffer + _packet_length) = (uint8_t)comm_type;
    _packet_length += 1;

    // set the data type
    // *(_packet_buffer + _packet_length) = (uint8_t)data_type;
    // _packet_length += 1;
    
    // set the payload length
    *(_packet_buffer + _packet_length) = (uint16_t)payload_len;
    _packet_length += 2;
    Serial.printf("_create_packet payload length %d\n", payload_len);
    
    // packet_number is set by the calling function and is member of the class
    *(_packet_buffer + _packet_length) = (uint16_t)_packet_number;
    _packet_length += 2;
    Serial.printf("_create_packet packet number %d\n", _packet_number);
        
    // Payload is already in the buffer, just increment the payload length
    // memcpy(_packet_buffer+_packet_length, payload, payload_len);
    _packet_length += payload_len;
    
    // finally copy the ending character
    *(_packet_buffer + _packet_length) = (uint8_t)END_CHARACTER;
    _packet_length += 1;
    
    // with this we have the packet, return back to the calling function.
    Serial.printf("_create_packet: length: %d\n", _packet_length);
}

/**
 * Give semaphore which indicates that queued data is transmitted. 
 */
void BluetoothCommunication::give_data_semaphore(){
    debug("give_data_semaphore: giving data written semaphore");
    xSemaphoreGive(_data_written_semaphore);
}
    