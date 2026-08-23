#include "range_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =====================================================
// EARTH RADIUS
// =====================================================

#define EARTH_RADIUS_M    6371000.0

// =====================================================
// DEGREE TO RADIAN CONVERSION
// =====================================================

static double degrees_to_radians(
    double degrees
)
{
    return degrees *
           (M_PI / 180.0);
}

// =====================================================
// PARSE SENSOR PAYLOAD
// =====================================================

bool range_parse_sensor_payload(
    const uint8_t *frame,
    int frame_length,
    sensor_packet_t *packet
)
{
    if (
        frame == NULL ||
        packet == NULL
    )
    {
        return false;
    }

    // The full received frame must be 40 bytes:
    //
    // Bytes 0-3   : RadioHead header
    // Bytes 4-39  : 36-byte sensor payload

    if (
        frame_length !=
        EXPECTED_LORA_FRAME_LENGTH
    )
    {
        return false;
    }

    char payload[
        SENSOR_PAYLOAD_LENGTH + 1
    ];

    memset(
        payload,
        0,
        sizeof(payload)
    );

    // Remove four RadioHead bytes
    memcpy(
        payload,
        frame +
        RADIOHEAD_HEADER_LENGTH,
        SENSOR_PAYLOAD_LENGTH
    );

    // Add string terminator
    payload[
        SENSOR_PAYLOAD_LENGTH
    ] = '\0';

    printf(
        "Parsed payload: %s\n",
        payload
    );

    // =================================================
    // EXPECTED PAYLOAD FORMAT
    // =================================================
    //
    // 01,000009,069145413,799577215,12,079
    //
    // Node ID       : 01
    // Sequence      : 000009
    // Latitude E7   : 069145413
    // Longitude E7  : 799577215
    // Satellites    : 12
    // HDOP x 100    : 079
    // =================================================

    unsigned int node_id = 0;

    unsigned long sequence = 0;

    long latitude_e7 = 0;

    long longitude_e7 = 0;

    unsigned int satellites = 0;

    unsigned int hdop_x100 = 0;

    int parsed_fields =
        sscanf(
            payload,
            "%2u,%6lu,%9ld,%9ld,%2u,%3u",
            &node_id,
            &sequence,
            &latitude_e7,
            &longitude_e7,
            &satellites,
            &hdop_x100
        );

    if (parsed_fields != 6)
    {
        printf(
            "Payload parsing failed. "
            "Parsed fields: %d\n",
            parsed_fields
        );

        return false;
    }

    // =================================================
    // FIELD VALIDATION
    // =================================================

    if (node_id > 99)
    {
        printf(
            "Invalid node ID\n"
        );

        return false;
    }

    if (sequence > 999999UL)
    {
        printf(
            "Invalid sequence number\n"
        );

        return false;
    }

    if (satellites > 99)
    {
        printf(
            "Invalid satellite count\n"
        );

        return false;
    }

    if (hdop_x100 > 999)
    {
        printf(
            "Invalid HDOP value\n"
        );

        return false;
    }

    // =================================================
    // CONVERT GPS COORDINATES
    // =================================================

    double latitude =
        ((double)latitude_e7) /
        10000000.0;

    double longitude =
        ((double)longitude_e7) /
        10000000.0;

    // Validate coordinate range
    if (
        latitude < -90.0 ||
        latitude > 90.0
    )
    {
        printf(
            "Invalid latitude\n"
        );

        return false;
    }

    if (
        longitude < -180.0 ||
        longitude > 180.0
    )
    {
        printf(
            "Invalid longitude\n"
        );

        return false;
    }

    // =================================================
    // COPY PARSED DATA
    // =================================================

    packet->node_id =
        (uint8_t)node_id;

    packet->sequence =
        (uint32_t)sequence;

    packet->latitude =
        latitude;

    packet->longitude =
        longitude;

    packet->satellites =
        (uint8_t)satellites;

    packet->hdop =
        ((double)hdop_x100) /
        100.0;

    return true;
}

// =====================================================
// HAVERSINE DISTANCE CALCULATION
// =====================================================

double range_calculate_distance_m(
    double gateway_latitude,
    double gateway_longitude,
    double node_latitude,
    double node_longitude
)
{
    double latitude_difference =
        degrees_to_radians(
            node_latitude -
            gateway_latitude
        );

    double longitude_difference =
        degrees_to_radians(
            node_longitude -
            gateway_longitude
        );

    double gateway_latitude_rad =
        degrees_to_radians(
            gateway_latitude
        );

    double node_latitude_rad =
        degrees_to_radians(
            node_latitude
        );

    double sin_latitude =
        sin(
            latitude_difference /
            2.0
        );

    double sin_longitude =
        sin(
            longitude_difference /
            2.0
        );

    double a =
        (
            sin_latitude *
            sin_latitude
        )
        +
        (
            cos(
                gateway_latitude_rad
            )
            *
            cos(
                node_latitude_rad
            )
            *
            sin_longitude
            *
            sin_longitude
        );

    /*
     * Protect against tiny floating-point rounding
     * errors outside the range 0 to 1.
     */
    if (a < 0.0)
    {
        a = 0.0;
    }

    if (a > 1.0)
    {
        a = 1.0;
    }

    double c =
        2.0 *
        atan2(
            sqrt(a),
            sqrt(1.0 - a)
        );

    return EARTH_RADIUS_M * c;
}

// =====================================================
// UPDATE PACKET STATISTICS
// =====================================================

bool range_update_statistics(
    packet_statistics_t *statistics,
    uint32_t sequence
)
{
    if (statistics == NULL)
    {
        return false;
    }

    // =================================================
    // FIRST RECEIVED PACKET
    // =================================================

    if (!statistics->initialized)
    {
        statistics->initialized =
            true;

        statistics->first_sequence =
            sequence;

        statistics->highest_sequence =
            sequence;

        statistics->expected_packets =
            1;

        statistics->received_packets =
            1;

        statistics->lost_packets =
            0;

        statistics->duplicate_packets =
            0;

        statistics->out_of_order_packets =
            0;

        statistics->packet_loss_percent =
            0.0;

        return true;
    }

    // =================================================
    // NEW SEQUENCE NUMBER
    // =================================================

    if (
        sequence >
        statistics->highest_sequence
    )
    {
        statistics->highest_sequence =
            sequence;

        statistics->received_packets++;

        statistics->expected_packets =
            statistics->highest_sequence -
            statistics->first_sequence +
            1U;

        if (
            statistics->expected_packets >
            statistics->received_packets
        )
        {
            statistics->lost_packets =
                statistics->expected_packets -
                statistics->received_packets;
        }
        else
        {
            statistics->lost_packets =
                0;
        }

        if (
            statistics->expected_packets >
            0
        )
        {
            statistics->packet_loss_percent =
                (
                    (double)
                    statistics->lost_packets *
                    100.0
                )
                /
                (double)
                statistics->expected_packets;
        }
        else
        {
            statistics->packet_loss_percent =
                0.0;
        }

        return true;
    }

    // =================================================
    // DUPLICATE PACKET
    // =================================================

    if (
        sequence ==
        statistics->highest_sequence
    )
    {
        statistics->duplicate_packets++;

        return false;
    }

    // =================================================
    // OUT-OF-ORDER PACKET
    // =================================================

    statistics->out_of_order_packets++;

    return false;
}

// =====================================================
// RELIABILITY CALCULATION
// =====================================================

bool range_is_reliable(
    const packet_statistics_t *statistics,
    double maximum_loss_percent,
    uint32_t minimum_expected_packets,
    int rssi,
    float snr
)
{
    if (statistics == NULL)
    {
        return false;
    }

    // Wait until enough packets have been evaluated
    if (
        statistics->expected_packets <
        minimum_expected_packets
    )
    {
        return false;
    }

    // Packet loss is the main reliability criterion
    if (
        statistics->packet_loss_percent >
        maximum_loss_percent
    )
    {
        return false;
    }

    // Reject extremely weak RSSI
    if (rssi < -120)
    {
        return false;
    }

    // Reject extremely poor SNR
    if (snr < -15.0f)
    {
        return false;
    }

    return true;
}