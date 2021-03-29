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
#include "time_manager.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Constructor for the BluetoothCommuninication Class
BluetoothCommunication::BluetoothCommunication(){
    if(_data_written_semaphore == NULL){
        _data_written_semaphore = xSemaphoreCreateBinary();
        if(_data_written_semaphore == NULL) {
            Serial.println("BluetoothCommunication: failed to create _data_written_semaphore");
        }
        xSemaphoreTake(_data_written_semaphore, 0);
    }
}

// Destructor for the BluetoothCommuninication Class
BluetoothCommunication::~BluetoothCommunication(){
    vSemaphoreDelete(_data_written_semaphore);
    _data_written_semaphore = NULL;
}


/**
 * Get response type name.
 * @param: uint8_t response_type_enum
 */
const char * _get_response_type_name(uint8_t type) {
    switch (type) {
        case RESPONSE_FOR_TIME_REQUEST:
            return "RESPONSE_FOR_TIME_REQUEST";
            
        case RESPONSE_FOR_IMAGE_INCOMING_REQUEST:
            return "RESPONSE_FOR_IMAGE_INCOMING_REQUEST";
        
        case RESPONSE_FOR_ARE_YOU_READY_REQUEST:
            return "RESPONSE_FOR_ARE_YOU_READY_REQUEST";

        case RESPONSE_FOR_IMAGE_SENT_REQUEST:
            return "RESPONSE_FOR_IMAGE_SENT_REQUEST";

        case RESPONSE_FOR_IMAGE_DATA:
            return "RESPONSE_FOR_IMAGE_DATA";

        case RESPONSE_FOR_OTHER_DATA:
            return "RESPONSE_FOR_OTHER_DATA";
    }
} 

/**
 * Verify response of a Bluetooth communication.
 * @param: Bluetooth pointer 
 * @param: Expected Bluetooth communication type.
 * @param: Expected Bluetooth response type.
 * @return: Boolean
 */
bool BluetoothCommunication::_verify_response(Bluetooth * my_bt, uint8_t comm_type, uint8_t check_category) {
    bool status = false;
    // we have received response.
    uint8_t * rcv_data;
    uint16_t rcv_length = my_bt->get_recv_buffer_length();
    Serial.printf("_verify_response: response length: %d\n", rcv_length);

    // allocate space to copy the response
    rcv_data = (uint8_t *) malloc(rcv_length * sizeof(uint8_t));
    
    if (rcv_data == NULL) {
        Serial.println("_verify_response: failed to allocate memory");
        return status;
    }

    // copy the response
    memcpy(rcv_data, my_bt->get_recv_buffer(), rcv_length);

    // release the recived data buffer mutex
    my_bt->give_rcv_data_mutex();

    if(rcv_data[0] == comm_type && rcv_data[1] == check_category){
        Serial.printf("_verify_response: response: %s\n", &rcv_data[_PREAMBLE_SIZE]);
        status = true;
    } else {
        Serial.printf("_verify_response: response error for %s\n", _get_response_type_name(check_category));
        status  = false;
    }

    // free the allocated space
    if (rcv_data != NULL) {
        free(rcv_data);
    }

    debug("\n\n");
    return status;
}

/**
 * Wait for the response from the phone on Bluetooth and verfies the response.
 * @param: Bluetooth object pointer
 * @return: boolean
 */
bool BluetoothCommunication::_wait_for_response(Bluetooth * my_bt) {
    bool status = false;

    // wait for the Semaphore for 50 ticks 
    if(my_bt->take_rcv_data_semaphore()) {
        debug("_wait_for_response: response received from phone");
        status = true;
    } else {
        Serial.println("_wait_for_response: no response received");
    }
    return status;
}

/**
 * Send data over Bluetooth. All other send functions calls this function to send data. 
 * 
 * @param: Bluetooth object pointer (must use pointer otherwise the )
 * @param: Bluetooth communication type
 * @param: Bluetooth communication category
 * @param: uint8_t * pointer to payload
 * @param: uint16_t payload length
 * @param: bool whether to wait for response or not.
 */
bool BluetoothCommunication::_send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, uint8_t category,
    const uint8_t * data_ptr, uint16_t data_length, bool response) {

    bool status = false;
    uint8_t tx_failed = 0;
    
    // create the data packet
    _create_packet(comm_type, category, data_ptr, data_length);

    // try three times if not successful
    while(true){
        // we have the data packet. send it over Bluetooth and wait for the response
        if(my_bt->bt_write_data(_packet_buffer, _packet_length) == _packet_length) {
            debug("_send_data: data sent succesully");
            status = true;

            // Do we wait for the response?
            if(response) {
                debug("_send_data: waiting for response");
                status = _wait_for_response(my_bt);

                if(!status) {
                    Serial.println("_send_data: wait for response time out");
                }
            }
            break;
        } else {
            Serial.printf("_send_data: failed to send data: %d\n", tx_failed);
            tx_failed++;
        }

        // try 3 times
        if(tx_failed == 3) {
            // don't need to wait for the semaphore
            return status;
        }
    }

    // wait for the semaphore
    debug("_send_data: waiting for data written semaphore");
    if (_data_written_semaphore != NULL) {
        if(xSemaphoreTake(_data_written_semaphore, ( TickType_t ) 10000) != pdTRUE){
            Serial.println("_send_data: failed to obtain data written semaphore");
        }
    }
    return status;
}


/**
 * Create the packet to be sent over Bluetooth.
 * @param: Bluetooth communication type
 * @param: uint8_t * pointer to payload
 * @param: uint16_t payload_len
 */
void BluetoothCommunication::_create_packet(_bluetooth_comm_type comm_type, uint8_t category, 
        const uint8_t * payload, uint16_t payload_len){

    _packet_length = 0;

    // is this request or data or response
    *(_packet_buffer + _packet_length) = (uint8_t)comm_type;
    _packet_length += 1;

    // set the packet category
    *(_packet_buffer + _packet_length) = (uint8_t)category;
    _packet_length += 1;
    
    // set the payload length
    // Serial.printf("_create_packet: payload length %d\n", payload_len);
    // Serial.printf("_create_packet: payload length %d --- %d\n", (uint8_t)((payload_len >> 8) & 0xFF), (uint8_t)(payload_len & 0xFF));
    // Serial.printf("_create_packet: payload length 0x%X --- 0x%X\n", (uint8_t)((payload_len >> 8) & 0xFF), (uint8_t)(payload_len & 0xFF));
    *(_packet_buffer + _packet_length) = (uint8_t)(payload_len & 0xFF);
    *(_packet_buffer + _packet_length + 1) = (uint8_t)((payload_len >> 8) & 0xFF);
    // Serial.printf("_create_packet: 0x%X ------- 0x%X\n", *(_packet_buffer + _packet_length), *(_packet_buffer + _packet_length + 1));
    // Serial.printf("_create_packet: %d ------- %d\n", *(_packet_buffer + _packet_length), *(_packet_buffer + _packet_length + 1));
    _packet_length += 2;
    
    // packet_number is set by the calling function and is member of the class
    // Serial.printf("_create_packet: packet number %d\n", _packet_number);
    // Serial.printf("_create_packet: packet number %d --- %d\n", (uint8_t)((_packet_number >> 8) & 0xFF), (uint8_t)(_packet_number & 0xFF));
    // Serial.printf("_create_packet: packet number 0x%X --- 0x%X\n", (uint8_t)((_packet_number >> 8) & 0xFF), (uint8_t)(_packet_number & 0xFF));
    *(_packet_buffer + _packet_length) = (uint8_t)(_packet_number & 0xFF);
    *(_packet_buffer + _packet_length + 1) = (uint8_t)((_packet_number >> 8) & 0xFF);
    // Serial.printf("_create_packet: 0x%X ------- 0x%X\n", *(_packet_buffer + _packet_length), *(_packet_buffer + _packet_length + 1));
    // Serial.printf("_create_packet: %d ------- %d\n", *(_packet_buffer + _packet_length), *(_packet_buffer + _packet_length + 1));
    _packet_length += 2;
        
    // Payload is already in the buffer, just increment the payload length
    if (payload != NULL) {
        // copy only if payload pointer is not NULL, in some case we already have data in the _packet_buffer and pass NULL pointer
        memcpy(_packet_buffer+_packet_length, payload, payload_len);
    }
    _packet_length += payload_len;
    
    // with this we have the packet, return back to the calling function.
    Serial.printf("_create_packet: payload length: %d, packet number: %d, length: %d\n", payload_len, _packet_number, _packet_length);
}


/**
 * Send incoming image request and verify the response.
 * @param Bluetooth * pointer
 * @return boolean
 */
bool BluetoothCommunication::_send_image_incoming_request(Bluetooth * my_bt) {
    bool status = false;

    // set the packet number
    _packet_number = 1;
    
    // send the image incoming request and verify the response.
    status = _send_data(my_bt, BT_REQUEST, IMAGE_INCOMING_REQUEST, (uint8_t *)_image_request, strlen(_image_request), true);
    if(status) {
        // request is sent, response is received, now verify the response.
        status = _verify_response(my_bt, BT_RESPONSE, (uint8_t)RESPONSE_FOR_IMAGE_INCOMING_REQUEST);
        if(!status) {
             Serial.println("_send_image_incoming_request: invalid response");
        }
    } else {
        Serial.println("_send_image_incoming_request: failed request");
    }

    return status;
}

/**
 * Send the are you ready request and verify the response.
 * @param: Bluetooth * pointer
 * @return Boolean
 */
bool BluetoothCommunication::_send_are_you_ready_request(Bluetooth * my_bt){
    bool status = false;

    // set the packet number
    _packet_number = 1;
    
    // send the are you ready request and verify the response.
    status |= _send_data(my_bt, BT_REQUEST, ARE_YOU_READY_REQUEST, (uint8_t *)_u_ready_request, strlen(_u_ready_request), true);
    if(status) {
        // request is sent, response is received, now verify the response.
        status = _verify_response(my_bt, BT_RESPONSE, (uint8_t)RESPONSE_FOR_ARE_YOU_READY_REQUEST);
        if(!status) {
             Serial.println("_send_are_you_ready_request: invalid response");
        }

    } else {
        Serial.println("_send_are_you_ready_request: failed request");
    }

    return status;
}

/**
 * Send image sent request and verify the response. 
 * @param: Bluetooth * pointer
 * @return: Boolean
 */
bool BluetoothCommunication::_send_image_sent_request(Bluetooth * my_bt, const char * file_name) {
    bool status = false;

    // set the packet number
    _packet_number = 1;
    
    // send the are you ready request and verify the response.
    status |= _send_data(my_bt, BT_REQUEST, IMAGE_SENT_REQUEST, (uint8_t *)file_name, strlen(file_name), true);
    if(status) {
        // request is sent, response is received, now verify the response.
        status = _verify_response(my_bt, BT_RESPONSE, (uint8_t)RESPONSE_FOR_IMAGE_SENT_REQUEST);
        if(!status) {
             Serial.println("_send_image_sent_request: invalid response");
        } 

    } else {
        Serial.println("_send_image_sent_request: failed request");
    }

    return status;
}

/**
 * This function completes the necessary steps required for image transfer. The procedure for image transfer are:
 *  1) Send the image incoming request.
 *  2) Wait for the response. On invalid response return false.
 *  3) Send are you ready request.
 *  4) Wait for the response. On invalid response return false.
 *  
 * @param Bluetooth object pointer
 * @return boolean True if all checks are passed else false.
 */
bool BluetoothCommunication::_image_transfer_confirmation(Bluetooth * my_bt){
    bool status  = false;

    // check if the camera is connected to phone or not.
    if(my_bt->get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        Serial.println("_image_transfer_confirmation: bt disconnected");
        return status;
    }

    // send the image incoming request and verify the response.
    status = _send_image_incoming_request(my_bt);
    if(status) {
        status = _send_are_you_ready_request(my_bt);
        if(!status) {
            Serial.println("_image_transfer_confirmation: failed at are you ready request");
        }
    } else {
        Serial.println("_image_transfer_confirmation: failed at image incoming request");
    }

    return status;
}


/**
 * Give semaphore which indicates that queued data is transmitted. 
 */
void BluetoothCommunication::give_data_semaphore(){
    if (_data_written_semaphore != NULL) {
        debug("give_data_semaphore: giving data written semaphore");
        xSemaphoreGive(_data_written_semaphore);
    }
}

/**
 * Send next image from the SD card to phone.
 * 
 * @param: Bluetooth object pointer
 * @param: FS object
 */
bool BluetoothCommunication::send_next_image(Bluetooth * my_bt, fs::FS &fs){
    bool status = false;

    if (my_bt == NULL) {
        Serial.println("send_next_image: null BT object");
        return status;
    }

    // check if the camera is connected to phone or not.
    if(my_bt->get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        Serial.println("send_next_image: bt disconnected");
        return status;
    }

    // open the root SD card directory
    File root_dir = fs.open("/");

    // check whether root is open and a directory
    if(root_dir == NULL || !root_dir.isDirectory()) { 
        Serial.println("send_next_image: failed to open the SD card");    
        return status;
    }

    // open the next file in the directory :: mechanism to check if there is any file or not.
    File my_file = root_dir.openNextFile();

    while(my_file){
        if(!my_file.isDirectory()){
            debug("send_next_image:  FILE: ");
            Serial.println(my_file.name());
            debug("send_next_image:  SIZE: ");
            Serial.println(my_file.size());
            break;
        }
        my_file = root_dir.openNextFile();
    }

    if(my_file.size() == 0) {
        // no need to proceed, because there is no file in the SD card to send.
        return status;
    }

    // now we have an image, start the Image transfer procedure.
    if(_image_transfer_confirmation(my_bt)) {
        debug("send_next_image: image transfer verified, sending image now...");
        
        // send the image file
        status = send_data_file(my_bt, IMAGE_DATA, &my_file);
        
        // send the image sent request
        if(status) {
            delay(100);
            status  = _send_image_sent_request(my_bt, my_file.name());
        }
        
        if(status) {
            // after the file is sent, delete it from SD card.
            Serial.printf("send_next_image: image file: %s sent\n", my_file.name());
            sd_delete_file(fs, my_file.name());
        }
    }

    // close the file
    my_file.close();
    root_dir.close();

    return status;
}

/**
 * Send the content of the file over Bluetooth. 
 * @param: Bluetooth object pointer
 * @param: Bluetooth data category
 * @param: FILE object pointer
 * 
 * @return: boolean
 */
bool BluetoothCommunication::send_data_file(Bluetooth * my_bt, _bluetooth_data_type data_type, File * my_file) {
    bool status = false;
    uint8_t response_category = RESPONSE_FOR_IMAGE_DATA;
    if (data_type == OTHER_DATA) {
        response_category = RESPONSE_FOR_OTHER_DATA;
    }

    if (my_bt == NULL) {
        Serial.println("send_data_file: null BT object");
        return status;
    }

    if(my_file == NULL) {
        Serial.println("send_data_file: null file pointer");
        return status;
    }

    uint32_t file_size = my_file->size();
    Serial.printf("send_data_file: file size %d\n", file_size);

    // First we need to check if we have the connection
    if(my_bt->get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        Serial.println("send_data_file: bt disconnected");
        return status;
    }

    // set the packet number
    _packet_number = 1;
    uint16_t read_size = 0;
    uint32_t total_bytes_sent = 0;

    while(my_file->available()){
        // read bytes from the file
        read_size = my_file->read(_packet_buffer + _PREAMBLE_SIZE, _PAYLOAD_SPACE);
        if(read_size == 0) {
            Serial.println("send_data_file: error reading file");
            break;
        }

        // send the read bytes to phone
        Serial.printf("send_data_file: read %d bytes\n", read_size);
        status = _send_data(my_bt, BT_DATA, (uint8_t)data_type, NULL, read_size, true);
        if(!status) {
            Serial.printf("send_data_file: tx failed, packet number %d\n", _packet_number);
            break;
        }

        // verify response.
        status = _verify_response(my_bt, BT_RESPONSE, response_category);
        if(!status) {
            Serial.println("send_data_file: invalid response");
            break;
        }

        // increment the packet number and total bytes sent 
        total_bytes_sent += read_size;
        _packet_number += 1;
    }

    if(status) {
        Serial.printf("send_data_file: out of %d bytes, %d sent\n", file_size, total_bytes_sent);
    }
    return status;
}

/**
 * Send the data over Bluetooth by creating a data packet and
 * receive the response.
 * 
 * @param Bluetooth object pointer
 * @param _bluetooth_comm_type communication type
 * @param category Communication category
 * @param uint8_t * pointer to the data array
 * @param uint16_t data length
 * 
 * @return boolean.
 */
bool BluetoothCommunication::send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, uint8_t category,
    const uint8_t * data_ptr, uint16_t data_length) {

    bool status = false;
    uint16_t total_data_length = data_length;

    // First we need to check if we have the connection
    if(my_bt->get_bt_connection_status() != BLUETOOTH_CONNECTED) {
        Serial.println("send_data: bt disconnected");
        return status;
    }

    // set the packet number to zero
    _packet_number = 255;

    // check the length, send the data, and receive response
    if (data_length > _PAYLOAD_SPACE) {
        uint16_t start = 0;
        uint16_t end = _PAYLOAD_SPACE;
        uint16_t sending_bytes = 0;

        // we need to divide data into several packets.
        while(total_data_length != 0){
            // keep sending until we reach the end of the data buffer
            // Serial.printf("sending packet number %d\n", _packet_number);
            sending_bytes = end - start;
            // Serial.printf("start: %d, end: %d, # bytes %d\n", start, end, sending_bytes);

            // copy the data into the buffer
            memcpy(_packet_buffer + _PREAMBLE_SIZE, data_ptr + start, sending_bytes);
            status = _send_data(my_bt, comm_type, category, NULL, sending_bytes, false);

            if(status) {
                total_data_length -= sending_bytes;
                start += sending_bytes;
                end += total_data_length < _PAYLOAD_SPACE ? total_data_length : _PAYLOAD_SPACE;
                _packet_number += 1;
            } else {
                // failed to send data, break free.
                Serial.printf("send_data: tx failed at packet number %d\n", _packet_number);
               break;
            }
        }
        Serial.printf("send_data: out of %d bytes, %d sent\n", data_length, end);

    } else {
        // copy the data into the buffer
        memcpy(_packet_buffer + _PREAMBLE_SIZE, data_ptr, data_length);
        status = _send_data(my_bt, comm_type, category, NULL, data_length, true);

    }

    return status;
}


/**
 * Send a Bluetooth request for current time. If response received, set the RTC time.
 * @param: Bluetooth object pointer
 * @return: boolean
 */
bool BluetoothCommunication::request_for_time(Bluetooth * my_bt){
    bool status = false;

    show_current_rtc_time();
    status = _send_data(my_bt, BT_REQUEST, TIME_REQUEST, (uint8_t *)_time_request, strlen(_time_request), true);
    if(status) {    
        uint8_t * rcv_data;
        uint16_t rcv_length = my_bt->get_recv_buffer_length();
        Serial.printf("request_for_time: response length: %d\n", rcv_length);

        // allocate space to copy the response
        rcv_data = (uint8_t *) malloc(rcv_length * sizeof(uint8_t));
        if (rcv_data == NULL) {
            Serial.println("request_for_time: failed to allocate memory");
            return false;
        }

        // copy the response
        memcpy(rcv_data, my_bt->get_recv_buffer(), rcv_length);

        // release the recived data buffer mutex
        my_bt->give_rcv_data_mutex();

        if(rcv_data[0] == BT_RESPONSE && rcv_data[1] == RESPONSE_FOR_TIME_REQUEST){
            // uint16_t data_length = (uint16_t)(rcv_data[2] | rcv_data[3] >> 8);
            // Serial.printf("data length %d\n", data_length);

            uint64_t time_in_millis;
            memcpy(&time_in_millis, &rcv_data[_PREAMBLE_SIZE], 8);
            Serial.printf("request_for_time: epoch time in millis: %llu\n", time_in_millis);
            // Serial.printf("%d\n", time_in_millis);
            
            // set the current timestamp
            set_rtc_time(time_in_millis);
        } else {
            Serial.println("request_for_time: invalid response");
            status = false;
        }
        
        // free the allocated space
        if (rcv_data != NULL) {
            free(rcv_data);
        }

        show_current_rtc_time();
    }

    return status;
}