#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lora_app.h"
#include "range_test.h"
#include "oled.h"

// =====================================================
// FIXED GATEWAY LOCATION
// =====================================================
//
// Replace these values with the final averaged
// coordinates of the fixed gateway position.
//
#define GATEWAY_LATITUDE_DEG       6.9155213
#define GATEWAY_LONGITUDE_DEG      79.9601458

// =====================================================
// RELIABILITY SETTINGS
// =====================================================

#define MINIMUM_TEST_PACKETS       20U
#define MAXIMUM_PACKET_LOSS        5.0

static const char *TAG = "GATEWAY";

// =====================================================
// MAIN
// =====================================================

void app_main(void)
{
    printf("\n");
    printf("====================================\n");
    printf(" LORA GPS RANGE TEST GATEWAY\n");
    printf("====================================\n");

    // =================================================
    // OLED INITIALIZATION
    // =================================================

    oled_init();

    oled_show_message(
        "LORA GATEWAY",
        "INITIALIZING..."
    );

    // =================================================
    // LORA INITIALIZATION
    // =================================================

    lora_app_init();

    oled_show_message(
        "LORA GATEWAY",
        "WAITING PACKET"
    );

    // =================================================
    // GATEWAY INFORMATION
    // =================================================d

    ESP_LOGI(
        TAG,
        "Gateway latitude : %.7f",
        GATEWAY_LATITUDE_DEG
    );

    ESP_LOGI(
        TAG,
        "Gateway longitude: %.7f",
        GATEWAY_LONGITUDE_DEG
    );

    ESP_LOGI(
        TAG,
        "Expected frame   : %d bytes",
        EXPECTED_LORA_FRAME_LENGTH
    );

    ESP_LOGI(
        TAG,
        "Expected payload : %d bytes",
        SENSOR_PAYLOAD_LENGTH
    );

    // =================================================
    // VARIABLES
    // =================================================

    uint8_t receive_buffer[64];

    packet_statistics_t statistics = {0};

    // =================================================
    // CSV HEADER FOR PYTHON
    // =================================================

    printf(
        "CSV_HEADER,"
        "node_id,"
        "sequence,"
        "distance_m,"
        "rssi_dbm,"
        "snr_db,"
        "frame_size_bytes,"
        "payload_size_bytes,"
        "expected_packets,"
        "received_packets,"
        "lost_packets,"
        "packet_loss_percent,"
        "reliable,"
        "node_latitude,"
        "node_longitude,"
        "satellites,"
        "hdop,"
        "duplicates,"
        "out_of_order,"
        "invalid_packets\n"
    );

    // =================================================
    // MAIN LOOP
    // =================================================

    while (1)
    {
        memset(
            receive_buffer,
            0,
            sizeof(receive_buffer)
        );

        // =============================================
        // RECEIVE LORA FRAME
        // =============================================

        int frame_length =
            lora_receive_packet(
                receive_buffer,
                sizeof(receive_buffer)
            );

        // No packet available
        if (frame_length == 0)
        {
            vTaskDelay(
                pdMS_TO_TICKS(10)
            );

            continue;
        }

        // LoRa receive error
        if (frame_length < 0)
        {
            statistics.invalid_packets++;

            ESP_LOGW(
                TAG,
                "LoRa receive error: %d",
                frame_length
            );

            oled_show_message(
                "LORA ERROR",
                "RX FAILED"
            );

            vTaskDelay(
                pdMS_TO_TICKS(500)
            );

            oled_show_message(
                "LORA GATEWAY",
                "WAITING PACKET"
            );

            continue;
        }

        // =============================================
        // GET RSSI AND SNR
        // =============================================

        int rssi =
            lora_get_packet_rssi();

        float snr =
            lora_get_packet_snr();

        // =============================================
        // PARSE SENSOR PACKET
        // =============================================

        sensor_packet_t packet = {0};

        bool valid_packet =
            range_parse_sensor_payload(
                receive_buffer,
                frame_length,
                &packet
            );

        // =============================================
        // INVALID PACKET
        // =============================================

        if (!valid_packet)
        {
            statistics.invalid_packets++;

            ESP_LOGW(
                TAG,
                "Invalid frame: received %d bytes, expected %d",
                frame_length,
                EXPECTED_LORA_FRAME_LENGTH
            );

            printf("RAW_FRAME,");

            for (
                int i = 0;
                i < frame_length;
                i++
            )
            {
                printf(
                    "%02X",
                    receive_buffer[i]
                );

                if (
                    i <
                    frame_length - 1
                )
                {
                    printf(" ");
                }
            }

            printf("\n");

            oled_show_message(
                "PACKET ERROR",
                "INVALID FRAME"
            );

            vTaskDelay(
                pdMS_TO_TICKS(500)
            );

            oled_show_message(
                "LORA GATEWAY",
                "WAITING PACKET"
            );

            continue;
        }

        // =============================================
        // UPDATE PACKET STATISTICS
        // =============================================

        bool new_packet =
            range_update_statistics(
                &statistics,
                packet.sequence
            );

        if (!new_packet)
        {
            ESP_LOGW(
                TAG,
                "Duplicate or out-of-order sequence: %lu",
                (unsigned long)packet.sequence
            );

            continue;
        }

        // =============================================
        // CALCULATE DISTANCE
        // =============================================

        double distance_m =
            range_calculate_distance_m(
                GATEWAY_LATITUDE_DEG,
                GATEWAY_LONGITUDE_DEG,
                packet.latitude,
                packet.longitude
            );

        // =============================================
        // CHECK RELIABILITY
        // =============================================

        bool reliable =
            range_is_reliable(
                &statistics,
                MAXIMUM_PACKET_LOSS,
                MINIMUM_TEST_PACKETS,
                rssi,
                snr
            );

        // =============================================
        // HUMAN-READABLE OUTPUT
        // =============================================

        printf("\n");
        printf("====================================\n");
        printf(" VALID RANGE TEST PACKET\n");
        printf("====================================\n");

        printf(
            "Node ID          : %u\n",
            packet.node_id
        );

        printf(
            "Sequence         : %lu\n",
            (unsigned long)packet.sequence
        );

        printf(
            "Node latitude    : %.7f\n",
            packet.latitude
        );

        printf(
            "Node longitude   : %.7f\n",
            packet.longitude
        );

        printf(
            "Satellites       : %u\n",
            packet.satellites
        );

        printf(
            "HDOP             : %.2f\n",
            packet.hdop
        );

        printf(
            "Distance         : %.2f m\n",
            distance_m
        );

        printf(
            "RSSI             : %d dBm\n",
            rssi
        );

        printf(
            "SNR              : %.2f dB\n",
            snr
        );

        printf(
            "Frame size       : %d bytes\n",
            frame_length
        );

        printf(
            "Payload size     : %d bytes\n",
            SENSOR_PAYLOAD_LENGTH
        );

        printf(
            "Expected packets : %lu\n",
            (unsigned long)
            statistics.expected_packets
        );

        printf(
            "Received packets : %lu\n",
            (unsigned long)
            statistics.received_packets
        );

        printf(
            "Lost packets     : %lu\n",
            (unsigned long)
            statistics.lost_packets
        );

        printf(
            "Packet loss      : %.2f %%\n",
            statistics.packet_loss_percent
        );

        printf(
            "Duplicates       : %lu\n",
            (unsigned long)
            statistics.duplicate_packets
        );

        printf(
            "Out of order     : %lu\n",
            (unsigned long)
            statistics.out_of_order_packets
        );

        printf(
            "Invalid packets  : %lu\n",
            (unsigned long)
            statistics.invalid_packets
        );

        printf(
            "Reliable         : %s\n",
            reliable ? "YES" : "NO"
        );

        printf("====================================\n");

        // =============================================
        // OLED UPDATE
        // =============================================

        oled_show_range_data(
            distance_m,
            rssi,
            snr,
            statistics.packet_loss_percent,
            statistics.received_packets,
            statistics.expected_packets,
            reliable
        );

        // =============================================
        // CSV OUTPUT FOR PYTHON
        // =============================================

        printf(
            "CSV_DATA,"
            "%u,"
            "%lu,"
            "%.2f,"
            "%d,"
            "%.2f,"
            "%d,"
            "%d,"
            "%lu,"
            "%lu,"
            "%lu,"
            "%.2f,"
            "%s,"
            "%.7f,"
            "%.7f,"
            "%u,"
            "%.2f,"
            "%lu,"
            "%lu,"
            "%lu\n",

            packet.node_id,

            (unsigned long)
            packet.sequence,

            distance_m,

            rssi,

            snr,

            frame_length,

            SENSOR_PAYLOAD_LENGTH,

            (unsigned long)
            statistics.expected_packets,

            (unsigned long)
            statistics.received_packets,

            (unsigned long)
            statistics.lost_packets,

            statistics.packet_loss_percent,

            reliable ? "YES" : "NO",

            packet.latitude,

            packet.longitude,

            packet.satellites,

            packet.hdop,

            (unsigned long)
            statistics.duplicate_packets,

            (unsigned long)
            statistics.out_of_order_packets,

            (unsigned long)
            statistics.invalid_packets
        );

        fflush(stdout);

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}