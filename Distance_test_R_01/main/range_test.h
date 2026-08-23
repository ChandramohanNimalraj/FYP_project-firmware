#ifndef RANGE_TEST_H
#define RANGE_TEST_H

#include <stdbool.h>
#include <stdint.h>

// =====================================================
// PACKET CONFIGURATION
// =====================================================

// Your LoRa library adds four RadioHead bytes
#define RADIOHEAD_HEADER_LENGTH       4

// Sensor-node ASCII payload
#define SENSOR_PAYLOAD_LENGTH         36

// 4-byte header + 36-byte payload
#define EXPECTED_LORA_FRAME_LENGTH    40

// =====================================================
// PARSED SENSOR PACKET
// =====================================================

typedef struct
{
    uint8_t node_id;

    uint32_t sequence;

    double latitude;

    double longitude;

    uint8_t satellites;

    double hdop;

} sensor_packet_t;

// =====================================================
// PACKET STATISTICS
// =====================================================

typedef struct
{
    bool initialized;

    uint32_t first_sequence;

    uint32_t highest_sequence;

    uint32_t expected_packets;

    uint32_t received_packets;

    uint32_t lost_packets;

    uint32_t duplicate_packets;

    uint32_t out_of_order_packets;

    uint32_t invalid_packets;

    double packet_loss_percent;

} packet_statistics_t;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================

// Remove the 4-byte RadioHead header and parse the
// 36-byte sensor payload.
bool range_parse_sensor_payload(
    const uint8_t *frame,
    int frame_length,
    sensor_packet_t *packet
);

// Calculate the straight-line ground distance between
// the fixed gateway and moving sensor node.
double range_calculate_distance_m(
    double gateway_latitude,
    double gateway_longitude,
    double node_latitude,
    double node_longitude
);

// Update packet statistics using the sequence number.
//
// Return:
// true  = new packet
// false = duplicate or out-of-order packet
bool range_update_statistics(
    packet_statistics_t *statistics,
    uint32_t sequence
);

// Determine whether communication is reliable.
bool range_is_reliable(
    const packet_statistics_t *statistics,
    double maximum_loss_percent,
    uint32_t minimum_expected_packets,
    int rssi,
    float snr
);

#endif