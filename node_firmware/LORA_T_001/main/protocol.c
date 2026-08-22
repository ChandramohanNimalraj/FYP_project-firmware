/*
 * protocol.c
 *
 *  Created on: May 12, 2026
 *      Author: ASUS
 */

#include <string.h>

#include "protocol.h"

// =====================================================
// CHECKSUM CALCULATION
// =====================================================
uint8_t protocol_calculate_checksum(
    uint8_t *buffer,
    uint16_t length
)
{
    uint8_t checksum = 0;

    // Sum all bytes
    for (int i = 0; i < length; i++)
    {
        checksum += buffer[i];
    }

    // Return 2's complement
    return -checksum;
}

// =====================================================
// CHECKSUM VALIDATION
// =====================================================
bool protocol_validate_checksum(
    uint8_t *buffer,
    uint16_t length
)
{
    uint8_t checksum = 0;

    // Sum all bytes
    for (int i = 0; i < length; i++)
    {
        checksum += buffer[i];
    }

    // Valid if total becomes 0
    return (checksum == 0);
}

// =====================================================
// CREATE SENSOR PACKET
// =====================================================
void protocol_create_sensor_packet(
    sensor_packet_t *packet,
    uint8_t sender_id,
    uint8_t receiver_id,
    uint8_t sequence,
    char *payload
)
{
    if (packet == NULL)
    {
        return;
    }

    // =================================================
    // CLEAR ENTIRE PACKET
    // =================================================
    memset(
        packet,
        0,
        sizeof(sensor_packet_t)
    );

    // =================================================
    // HEADER
    // =================================================
    packet->_.start_byte =
        PACKET_START_BYTE;

    packet->_.sender_id =
        sender_id;

    packet->_.receiver_id =
        receiver_id;

    packet->_.packet_type =
        PACKET_TYPE_SENSOR_DATA;

    packet->_.sequence =
        sequence;

    // =================================================
    // PAYLOAD
    // =================================================
    if (payload != NULL)
    {
        // Fill payload with spaces
        // instead of NULL bytes
        memset(
            packet->_.payload,
            ' ',
            sizeof(packet->_.payload)
        );

        // Copy actual payload bytes
        memcpy(
            packet->_.payload,
            payload,
            strlen(payload)
        );

        // Save payload length
        packet->_.payload_length =
            strlen(payload);
    }

    // =================================================
    // END BYTE
    // =================================================
    packet->_.end_byte =
        PACKET_END_BYTE;

    // =================================================
    // CHECKSUM
    // =================================================
    packet->_.checksum =
        protocol_calculate_checksum(
            packet->buffer,
            SENSOR_PACKET_LENGTH - 2
        );
}

// =====================================================
// CREATE ACK PACKET
// =====================================================
void protocol_create_ack_packet(
    ack_packet_t *packet,
    uint8_t gateway_id,
    uint8_t node_id,
    uint8_t sequence,
    uint8_t ack_status
)
{
    if (packet == NULL)
    {
        return;
    }

    // =================================================
    // CLEAR PACKET
    // =================================================
    memset(
        packet,
        0,
        sizeof(ack_packet_t)
    );

    // =================================================
    // ACK DATA
    // =================================================
    packet->_.start_byte =
        PACKET_START_BYTE;

    packet->_.gateway_id =
        gateway_id;

    packet->_.node_id =
        node_id;

    packet->_.sequence =
        sequence;

    packet->_.ack_status =
        ack_status;

    // =================================================
    // CHECKSUM
    // =================================================
    packet->_.checksum =
        protocol_calculate_checksum(
            packet->buffer,
            ACK_PACKET_LENGTH - 1
        );
}