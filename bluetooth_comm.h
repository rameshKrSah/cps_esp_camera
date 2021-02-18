#ifndef __BLUETOOTH_COMM__
#define __BLUETOOTH_COMM__

#include "Arduino.h"
#include "bluetooth.h"

typedef enum {
    IMAGE_DATA = 0,
    GENERAL_DATA
}bluetooth_comm_data_type;


class BluetoothCommunication{
    private:
    static const char END_CHARACTER = '#';

    static const uint8_t IMAGE_TYPE_IDENTIFIER = 0xA0;
    static const uint8_t GENERAL_TYPE_IDENTIFIER = 0xB0;

    uint8_t _packet_buffer[MAX_LENGTH];
    uint8_t _packet_number = 0;
    uint8_t _packet_length = 0;


    /**
     * Create the packet to be sent over Bluetooth.
     * 
     * @param: bluetooth_comm_data_type
     * @param: uint8_t * pointer to payload
     * @param: uint8_t payload_len
     */
    void _create_packet(const uint8_t * payload, uint8_t payload_len, bluetooth_comm_data_type data_type);

    /**
     * Wait for the response from the phone on Bluetooth and verfies the response.
     * @param: bluetooth_comm_data_type
     * @param: Bluetooth my_bt
     * @return: boolean
     */
    bool _wait_for_response(Bluetooth my_bt, bluetooth_comm_data_type data_type);

    bool _send_data(Bluetooth my_bt, bluetooth_comm_data_type data_type, const uint8_t* data_ptr, uint8_t data_length);

    public:
    BluetoothCommunication();
    ~BluetoothCommunication();

    /**
     * Send the data over Bluetooth by creating a data packet and
     * receive the response.
     * 
     * @param Bluetooth object
     * @param bluetooth_comm_data_type data type enum
     * @param uint8_t * pointer to the data array
     * @param uint8_t data length
     * 
     * @return boolean.
     */
    bool send_data(Bluetooth my_bt, 
        bluetooth_comm_data_type data_type,
        const uint8_t * data_ptr, uint8_t data_length);
};


#endif