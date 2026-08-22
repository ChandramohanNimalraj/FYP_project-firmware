#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ack_packet.h"

// ================= START & END BYTES =================
#define PACKET_START_BYTE      0xAA
#define PACKET_END_BYTE        0x55

// ================= NODE IDs =================
#define GATEWAY_ID             0x07

#define NODE_1_ID              0x01
#define NODE_2_ID              0x02

// ================= PACKET TYPES =================
#define PACKET_TYPE_SENSOR_DATA    0x01
#define PACKET_TYPE_ACK            0x02
#define PACKET_TYPE_ERROR          0x03

// ================= SENSOR PACKET =================
#define SENSOR_PACKET_LENGTH   16

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

        // Payload data
        char payload[8];

        // Checksum
        uint8_t checksum;

        // End byte
        uint8_t end_byte;

    }_;

    // Raw byte buffer
    uint8_t buffer[SENSOR_PACKET_LENGTH];

} sensor_packet_t;

// ================= CHECKSUM FUNCTIONS =================
uint8_t protocol_calculate_checksum(
    uint8_t *buffer,
    uint16_t length
);

bool protocol_validate_checksum(
    uint8_t *buffer,
    uint16_t length
);

// ================= PACKET CREATION =================
void protocol_create_sensor_packet(
    sensor_packet_t *packet,
    uint8_t sender_id,
    uint8_t receiver_id,
    uint8_t sequence,
    char *payload
);

void protocol_create_ack_packet(
    ack_packet_t *packet,
    uint8_t gateway_id,
    uint8_t node_id,
    uint8_t sequence,
    uint8_t ack_status
);

#endif