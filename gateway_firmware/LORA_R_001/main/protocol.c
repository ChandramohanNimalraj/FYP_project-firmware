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

    // Return 2's complement checksum
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

    // Exclude END BYTE
    for (int i = 0; i < length - 1; i++)
    {
        checksum += buffer[i];
    }

    return (checksum == 0);
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

    // Clear packet memory
    memset(
        packet,
        0,
        sizeof(ack_packet_t)
    );

    // ================= PACKET DATA =================
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

    // ================= CHECKSUM =================
    packet->_.checksum =
        protocol_calculate_checksum(
            packet->buffer,
            ACK_PACKET_LENGTH - 1
        );
}