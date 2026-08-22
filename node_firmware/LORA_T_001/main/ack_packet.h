#ifndef ACK_PACKET_H
#define ACK_PACKET_H

#include <stdint.h>

// ================= ACK STATUS =================
#define ACK_STATUS_SUCCESS     0x01
#define ACK_STATUS_ERROR       0x02
#define ACK_STATUS_BUSY        0x03

// ================= ACK PACKET LENGTH =================
#define ACK_PACKET_LENGTH      6

// ================= ACK PACKET =================
typedef union
{
    struct
    {
        // Start byte
        uint8_t start_byte;

        // Gateway ID
        uint8_t gateway_id;

        // Sensor node ID
        uint8_t node_id;

        // Packet sequence number
        uint8_t sequence;

        // ACK status
        uint8_t ack_status;

        // Checksum
        uint8_t checksum;

    }_;

    // Raw packet buffer
    uint8_t buffer[ACK_PACKET_LENGTH];

} ack_packet_t;

#endif