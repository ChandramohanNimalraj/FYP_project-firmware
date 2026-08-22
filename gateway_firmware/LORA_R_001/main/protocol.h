#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// =====================================================
// START & END BYTES
// =====================================================
#define PACKET_START_BYTE      0xAA
#define PACKET_END_BYTE        0x55

// =====================================================
// NODE IDs
// =====================================================
#define GATEWAY_ID             0x07

#define NODE_1_ID              0x01
#define NODE_2_ID              0x02

// =====================================================
// PACKET TYPES
// =====================================================
#define PACKET_TYPE_DATA       0x01
#define PACKET_TYPE_ACK        0x02
#define PACKET_TYPE_ERROR      0x03

// =====================================================
// ACK STATUS
// =====================================================
#define ACK_STATUS_SUCCESS     0x01
#define ACK_STATUS_ERROR       0x02
#define ACK_STATUS_BUSY        0x03

// =====================================================
// ACK PACKET LENGTH
// =====================================================
#define ACK_PACKET_LENGTH      6

// =====================================================
// ACK PACKET
// =====================================================
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

    // Raw byte buffer
    uint8_t buffer[ACK_PACKET_LENGTH];

} ack_packet_t;

// =====================================================
// CHECKSUM FUNCTIONS
// =====================================================
uint8_t protocol_calculate_checksum(
    uint8_t *buffer,
    uint16_t length
);

bool protocol_validate_checksum(
    uint8_t *buffer,
    uint16_t length
);

// =====================================================
// ACK PACKET FUNCTIONS
// =====================================================
void protocol_create_ack_packet(
    ack_packet_t *packet,
    uint8_t gateway_id,
    uint8_t node_id,
    uint8_t sequence,
    uint8_t ack_status
);

#endif