#ifndef __BLUETOOTH_COMM__
#define __BLUETOOTH_COMM__

#include "Arduino.h"
#include "bluetooth.h"
#include "FS.h"

// typedef enum {
//     IMAGE_DATA = 0xA0,
//     GENERAL_DATA = 0xB0
// }bluetooth_comm_data_type;


/**
 * Bluetooth data is sent and received in packets, with each packet having header information and data payload.
 * The first byte tells whether this transmission is a BLUETOOTH_REQUEST, DATA_TRANSFER, or a RESPONSE_PACKET.
 * The second byte futher distills the request, data, or reponse into different categories.
 * The next two bytes carries the length of the payload.
 * The next two bytes carries the packet number for the data payload.
 * Remaining bytes carries the data payload.
 * 
 * -----------------------------------------------------------------------------------------------------------------
 * | BLUETOOTH_REQUEST or DATA_TRANSFER or RESPONSE_PACKET | CATEGORIES | PAYLOAD LENGTH | PACKET NUMBER | PAYLOAD |
 * -----------------------------------------------------------------------------------------------------------------
 * 
 */

typedef enum {
    BT_REQUEST = 0x0A,
    BT_DATA = 0x0B,
    BT_RESPONSE = 0x0C
}_bluetooth_comm_type;

typedef enum {
    TIME_REQUEST = 0x00,
    IMAGE_INCOMING_REQUEST = 0x01,
    ARE_YOU_READY_REQUEST = 0x02,
    IMAGE_SENT_REQUEST = 0x03
}_bluetooth_request_type; 

typedef enum {
    IMAGE_DATA = 0x00,
    OTHER_DATA = 0x01
}_bluetooth_data_type;

typedef enum {
    RESPONSE_FOR_TIME_REQUEST = 0x00,
    RESPONSE_FOR_IMAGE_INCOMING_REQUEST = 0x01,
    RESPONSE_FOR_ARE_YOU_READY_REQUEST = 0x02,
    RESPONSE_FOR_IMAGE_SENT_REQUEST = 0x03,
    RESPONSE_FOR_IMAGE_DATA = 0x04,
    RESPONSE_FOR_OTHER_DATA = 0x05
}_bluetooth_response_type;


static const char * _time_request = "time please";
static const char * _image_request = "image incoming";
static const char * _u_ready_request = "are you ready";
static const char * _image_sent_request = "image sent";

static const char * _am_ready_response = "i am ready";
static const char * _ok_response = "ok";
static const char * _image_received_response = "image received";


class BluetoothCommunication {
    private:
    SemaphoreHandle_t _data_written_semaphore = NULL;
    
    // comm type (1), categories(1), payload length byte (2), packet number (2)
    static const uint8_t _PREAMBLE_SIZE = 6;
    static const uint16_t _PAYLOAD_SPACE = MAX_LENGTH - _PREAMBLE_SIZE;

    uint16_t _packet_number = 0;
    uint16_t _packet_length = 0;
    uint8_t _packet_buffer[MAX_LENGTH + 1];

    /**
     * Create the packet to be sent over Bluetooth.
     * @param: _bluetooth_comm_type comm_type
     * @param: Bluetooth communication category
     * @param: uint8_t * pointer to payload
     * @param: uint16_t payload_len
     */
    void _create_packet(_bluetooth_comm_type comm_type, uint8_t category, const uint8_t * payload, 
            uint16_t payload_len);


    /**
     * Wait for the response from the phone on Bluetooth and verfies the response.
     * @param: Bluetooth object pointer
     * @return: boolean
     */
    bool _wait_for_response(Bluetooth * my_bt);

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
    bool _send_data(Bluetooth * my_bt, _bluetooth_comm_type comm_type, uint8_t category,
        const uint8_t * data_ptr, uint16_t data_length, bool response);

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
    bool _image_transfer_confirmation(Bluetooth * my_bt);

    /**
     * Send the are you ready request and verify the response.
     * @param: Bluetooth * pointer
     * @return Boolean
     */
    bool _send_are_you_ready_request(Bluetooth * my_bt);

    /**
     * Send incoming image request and verify the response.
     * @param Bluetooth * pointer
     * @return boolean
     */
    bool _send_image_incoming_request(Bluetooth * my_bt);

    /**
     * Send image sent request and verify the response. 
     * @param: Bluetooth * pointer
     * @return: Boolean
     */
    bool _send_image_sent_request(Bluetooth * my_bt);

    /**
     * Verify response of a Bluetooth communication.
     * @param: Bluetooth pointer 
     * @param: Expected Bluetooth communication type.
     * @param: Expected Bluetooth response type.
     * @return: Boolean
     */
    bool _verify_response(Bluetooth * my_bt, uint8_t comm_type, uint8_t check_category);

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
     * @param: Bluetooth data category
     * @param: FILE object pointer
     * 
     * @return: boolean
     */
    bool send_data_file(Bluetooth * my_bt, _bluetooth_data_type data_type, File * my_file);

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

    /**
     * Send a request for current time to the phone. If response received set the RTC time. 
     */
    bool request_for_time(Bluetooth * my_bt);
};


#endif