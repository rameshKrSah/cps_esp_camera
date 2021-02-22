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

// Constructor and Destructor
BluetoothCommunication::BluetoothCommunication(){}
BluetoothCommunication::~BluetoothCommunication(){}


/**
 * Send the content of the file over Bluetooth. 
 * @param: Bluetooth object
 * @param: bluetooth_comm_data_type data type
 * @param: FILE object
 * 
 * @return: boolean
 */
bool BluetoothCommunication::send_data_file(Bluetooth my_bt, bluetooth_comm_data_type data_type, File my_file) {
    bool status = false;
    
    if(my_file == NULL) {
        return status;
    }

    // First we need to check if we have the connection
    // if(my_bt.get_bt_connection_status() != BLUETOOTH_CONNECTED) {
    //     debug("send_data::camera not connected to phone");
    //     return status;
    // }

    // data type byte (1), payload length byte (2), packet number (4), and end character (1)
    uint16_t empty_packet_space = MAX_LENGTH - 8;

    // set the packet number and allocate memory for the data packet (MAX_LENGTH) bytes
    _packet_number = 1;

    // _temp_buffer_buffer = (uint8_t *) malloc(empty_packet_space * sizeof(uint8_t));
    uint16_t read_size = 0;

    while(true){
        // read bytes from the file
        read_size = my_file.read(_packet_buffer + 7, empty_packet_space);
        Serial.printf("size %d, position %d, packet number %d\n", my_file.size(), my_file.position(), _packet_number);
        Serial.printf("read %d bytes from file\n", read_size);
        // Serial.printf("%s\n", (char *)_packet_buffer);

        // send the read bytes to phone
        // status = _send_data(my_bt, data_type, NULL, read_size, false);
        _packet_number += 1;
        break;
    }

    // free the allocated memory
    // if (_temp_buffer_buffer != NULL) {
    //     debug("freeing the allocated memory");
    //     // free the allocated memory
    //     free(_temp_buffer_buffer);
    // }

    debug("returning from the send function");
    return status;
}

/**
 * Send the data over Bluetooth by creating a data packet and
 * receive the response.
 * 
 * @param Bluetooth object
 * @param bluetooth_comm_data_type data type enum
 * @param uint8_t * pointer to the data array
 * @param uint32_t data length
 * 
 * @return boolean.
 */
bool BluetoothCommunication::send_data(Bluetooth my_bt, bluetooth_comm_data_type data_type, 
    const uint8_t* data_ptr, uint16_t data_length) {
    bool status = false;
    uint8_t total_data_length = data_length;

    // First we need to check if we have the connection
    if(my_bt.get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        debug("send_data::camera not connected to phone");
        return status;
    }

    /**
     * Since we can only send 330 bytes at a time and based on the data type we have filled or empty
     * 3rd byte position we need to calculate the number of free space available from the 330 bytes.
     */
    uint8_t empty_packet_space = MAX_LENGTH - 4; // data type byte (1), payload length byte(2), and end character(1)
    if (data_type == IMAGE_DATA) {
        empty_packet_space = empty_packet_space - 4; // if image data then packet sequence number (4 bytes) also
    }

    // set the packet number to zero
    _packet_number = 1;
    Serial.printf("total data length %d\n", total_data_length);

    // check the length, send the data, and receive response
    if (data_length > empty_packet_space) {
        // we need to divide data into several packets.
        uint8_t start = 0;
        uint8_t end = empty_packet_space;
        uint8_t sending_bytes = 0;

        // keep sending until we reach the end of the data buffer
        while(end < total_data_length){
            Serial.printf("sending packet number %d\n", _packet_number);
            sending_bytes = end-start;
            Serial.printf("start: %d, end: %d, # bytes %d\n", start, end, sending_bytes);

            status = _send_data(my_bt, data_type, data_ptr+start, sending_bytes, true);
            if(status) {
                data_length -= sending_bytes;
                start += end;
                end += data_length < empty_packet_space ? data_length : empty_packet_space;
                _packet_number += 1;
            } else {
               break;
            }
        }
    } else {
        status = _send_data(my_bt, data_type, data_ptr, data_length, true);
    }

    return status;
}

/**
 * Send data and try 3 times if not succesully.
 */
bool BluetoothCommunication::_send_data(Bluetooth my_bt, bluetooth_comm_data_type data_type, 
    const uint8_t * data_ptr, uint16_t data_length, bool response) {
    bool status = false;
    uint8_t tx_failed = 0;
    
    // we can send all the data at once
    _create_packet(data_ptr, data_length, data_type);
    // return false;

    while(true){
        // we have the data packet. send it over Bluetooth and wait for the response
        if(my_bt.bt_write_data(_packet_buffer, _packet_length) == _packet_length) {
            debug("data sent succesully");
            status = true;

            // Do we wait for the response?
            if(response) {
                debug("waiting for the response");
                status = _wait_for_response(my_bt, data_type);
            }
            break;
        } else {
            debug("failed to send data over bluetooth");
            tx_failed++;
        }

        // try 3 times
        if(tx_failed == 3) {
            break;
        }
        
    }

    return status;
}

/**
 * Wait for the response from the phone on Bluetooth and verfies the response.
 * @param: bluetooth_comm_data_type
 * @return: boolean
 */
bool BluetoothCommunication::_wait_for_response(Bluetooth my_bt, bluetooth_comm_data_type data_type) {
    // wait for the Semaphore for 50 ticks 
    if(my_bt.take_rcv_data_semaphore()) {
        debug("response received from phone");
        // if we reach here, then we have received response and it is in the receive buffer.
        if(my_bt.get_recv_buffer() == data_type){
            // data was sent succesully
            debug("data type matched!!");
            return true;
        }
    }

    // data transmission Failed
    return false;
}

/**
 * Create the packet to be sent over Bluetooth.
 * 
 * @param: bluetooth_comm_data_type
 * @param: uint8_t * pointer to payload
 * @param: uint8_t payload_len
 */
void BluetoothCommunication::_create_packet(const uint8_t * payload, uint16_t payload_len, 
    bluetooth_comm_data_type data_type){

    _packet_length = 0;
    uint8_t * packet_bf_ptr = _packet_buffer;

    // set the data type
    *(packet_bf_ptr + _packet_length) = data_type;
    _packet_length += 1;

    // set the payload length
    *(_packet_buffer + _packet_length) = payload_len;
    _packet_length += 2;

    // packet_number is set by the calling function and is member of the class
    *(_packet_buffer + _packet_length) = _packet_number;
    _packet_length += 4;
    
    // now copy the payload 
    // memcpy(_packet_buffer+_packet_length, payload, payload_len);
    _packet_length += payload_len;

    // finally copy the ending character
    *(_packet_buffer + _packet_length) = (uint8_t)END_CHARACTER;
    _packet_length += 1;

    // with this we have the packet, return back to the calling function.
    Serial.printf("packet created, length; %d\n", _packet_length);
}
    