#ifndef DATA_PACKET_H
#define DATA_PACKET_H

#include <stdint.h>

// =====================================================
// DATA PACKET LENGTH
// =====================================================
#define DATA_PACKET_LENGTH     16

// =====================================================
// DATA PACKET
// =====================================================
typedef union
{
    struct
    {
        // Start byte
        uint8_t start_byte;

        // Sender node ID
        uint8_t sender_id;

        // Receiver ID
        uint8_t receiver_id;

        // Packet type
        uint8_t packet_type;

        // Packet sequence number
        uint8_t sequence;

        // Payload length
        uint8_t payload_length;

        // Sensor payload
        char payload[8];

        // Checksum
        uint8_t checksum;

        // End byte
        uint8_t end_byte;

    }_;

    // Raw byte buffer
    uint8_t buffer[DATA_PACKET_LENGTH];

} data_packet_t;

#endif