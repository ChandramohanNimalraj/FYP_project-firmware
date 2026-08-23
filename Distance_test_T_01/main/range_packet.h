#ifndef RANGE_PACKET_H
#define RANGE_PACKET_H

#include <stdint.h>

// =====================================================
// FIXED PACKET CONFIGURATION
// =====================================================

#define RANGE_PACKET_MAGIC       0xA55A
#define RANGE_PACKET_VERSION     1
#define RANGE_PACKET_SIZE        32

// =====================================================
// FIXED 32-BYTE RANGE TEST PACKET
// =====================================================

typedef struct __attribute__((packed))
{
    // Bytes 0-1
    uint16_t magic;

    // Byte 2
    uint8_t version;

    // Byte 3
    uint8_t node_id;

    // Bytes 4-7
    uint32_t sequence;

    // Bytes 8-11
    // Latitude multiplied by 10,000,000
    int32_t latitude_e7;

    // Bytes 12-15
    // Longitude multiplied by 10,000,000
    int32_t longitude_e7;

    // Bytes 16-19
    // Altitude in centimetres
    int32_t altitude_cm;

    // Bytes 20-23
    // UTC in HHMMSS form
    uint32_t utc_hhmmss;

    // Bytes 24-25
    // HDOP multiplied by 100
    uint16_t hdop_x100;

    // Byte 26
    uint8_t satellites;

    // Byte 27
    uint8_t gps_valid;

    // Bytes 28-29
    uint16_t reserved;

    // Bytes 30-31
    uint16_t crc16;

} range_packet_t;

// Compile-time packet-size check
_Static_assert(
    sizeof(range_packet_t) == RANGE_PACKET_SIZE,
    "Range packet must be exactly 32 bytes"
);

// =====================================================
// CRC-16 CCITT
// =====================================================

static inline uint16_t range_crc16_ccitt(
    const uint8_t *data,
    uint16_t length
)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

static inline void range_packet_update_crc(
    range_packet_t *packet
)
{
    packet->crc16 = 0;

    packet->crc16 =
        range_crc16_ccitt(
            (const uint8_t *)packet,
            RANGE_PACKET_SIZE - sizeof(packet->crc16)
        );
}

#endif