#ifndef __BLUETOOTH_COMM__
#define __BLUETOOTH_COMM__

#include "Arduino.h"
#include "bluetooth.h"
#include "FS.h"

typedef enum {
    IMAGE_DATA = 0xA0,
    GENERAL_DATA = 0xB0
}bluetooth_comm_data_type;

typedef enum {
    BT_REQUEST = 0x00,
    BT_DATA = 0x0A
}_bluetooth_comm_type;


class BluetoothCommunication {
    private:
    SemaphoreHandle_t _data_written_semaphore = NULL;
    static const uint8_t END_CHARACTER = '#';

    // comm type (1), payload length byte (2), packet number (2), and end character (1)
    static const uint8_t _PREAMBLE_SIZE = 5;
    static const uint16_t _PAYLOAD_SPACE = MAX_LENGTH - _PREAMBLE_SIZE + 1;

    // static const uint8_t IMAGE_TYPE_IDENTIFIER = 0xA0;
    // static const uint8_t GENERAL_TYPE_IDENTIFIER = 0xB0;

    uint16_t _packet_number = 0;
    uint16_t _packet_length = 0;
    uint8_t * _temp_buffer_buffer = NULL;
    uint8_t _packet_buffer[MAX_LENGTH + 1];


    /**
     * Create the packet to be sent over Bluetooth.
     * @param: _bluetooth_comm_type comm_type
     * @param: uint8_t * pointer to payload
     * @param: uint16_t payload_len
     */
    void _create_packet(_bluetooth_comm_type comm_type, const uint8_t * payload, 
            uint16_t payload_len);


    /**
     * Wait for the response from the phone on Bluetooth and verfies the response.
     * @param: Bluetooth communication type
     * @param: Bluetooth my_bt
     * @return: boolean
     */
    bool _wait_for_response(Bluetooth * my_bt, _bluetooth_comm_type comm_type);

    /**
     * Send data over Bluetooth. All other send functions calls this function to send data. 
     * 
     * @param: Bluetooth object pointer (must use pointer otherwise the )
     * @param: Bluetooth communication type
     * @param: uint8_t * pointer to payload
     * @param: uint16_t payload length
     * @param: bool whether to wait for response or not.
     */
    bool _send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, const uint8_t * data_ptr, 
        uint16_t data_length, bool response);

    public:

    BluetoothCommunication();
    ~BluetoothCommunication();

    /**
     * Send the data over Bluetooth by creating a data packet and
     * receive the response.
     * 
     * @param Bluetooth object pointer
     * @param bluetooth_comm_data_type data type enum
     * @param uint8_t * pointer to the data array
     * @param uint16_t data length
     * 
     * @return boolean.
     */
    bool send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, const uint8_t * data_ptr, uint16_t data_length);

    /**
     * Send the content of the file over Bluetooth. 
     * @param: Bluetooth object pointer
     * @param: Bluetooth communication type.
     * @param: File object pointer
     * 
     * @return: boolean
     */
    bool send_data_file(Bluetooth * my_bt, _bluetooth_comm_type comm_type, File * my_file);

    /**
     * Send next image from the SD card to phone.
     * 
     * @param: Bluetooth object pointer
     * @param: FS object
     */
    bool send_next_image(Bluetooth * my_bt, fs::FS &fs);

    /**
     * Give semaphore which indicates that queued data is transmitted. 
     */
    void give_data_semaphore();
};


#endif