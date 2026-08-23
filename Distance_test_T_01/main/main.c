#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gps_app.h"
#include "lora_app.h"

// =====================================================
// SENSOR NODE CONFIGURATION
// =====================================================

#define SENSOR_NODE_ID          1

#define TRANSMIT_INTERVAL_MS    1000

#define TEST_PACKET_LENGTH      36

static const char *TAG =
    "SENSOR_NODE";

// =====================================================
// MAIN
// =====================================================

void app_main(void)
{
    printf("\n");
    printf("====================================\n");
    printf(" LORA GPS RANGE TEST SENSOR NODE\n");
    printf("====================================\n");

    gps_app_init();

    lora_app_init();

    uint32_t sequence_number = 0;

    ESP_LOGI(
        TAG,
        "Sensor node started"
    );

    while (1)
    {
        gps_fix_t gps_fix = {0};

        bool gps_valid =
            gps_get_latest_fix(
                &gps_fix
            );

        char packet_text[64];

        int32_t latitude_e7 = 0;
        int32_t longitude_e7 = 0;

        uint8_t satellites = 0;
        uint16_t hdop_x100 = 0;

        if (gps_valid)
        {
            latitude_e7 =
                (int32_t)llround(
                    gps_fix.latitude *
                    10000000.0
                );

            longitude_e7 =
                (int32_t)llround(
                    gps_fix.longitude *
                    10000000.0
                );

            satellites =
                gps_fix.satellites;

            hdop_x100 =
                (uint16_t)llround(
                    gps_fix.hdop *
                    100.0
                );
        }

        /*
         * Fixed-format ASCII packet:
         *
         * NN,SSSSSS,LLLLLLLLL,OOOOOOOOO,GG,HHH
         *
         * NN         = node ID, 2 digits
         * SSSSSS     = sequence, 6 digits
         * LLLLLLLLL  = latitude E7, 9 digits
         * OOOOOOOOO  = longitude E7, 9 digits
         * GG         = satellites, 2 digits
         * HHH        = HDOP x 100, 3 digits
         */

        snprintf(
            packet_text,
            sizeof(packet_text),
            "%02u,%06lu,%09ld,%09ld,%02u,%03u",
            SENSOR_NODE_ID,
            (unsigned long)(
                sequence_number % 1000000UL
            ),
            (long)latitude_e7,
            (long)longitude_e7,
            satellites,
            hdop_x100
        );

        size_t actual_length =
            strlen(packet_text);

        ESP_LOGI(
            TAG,
            "Sequence      : %lu",
            (unsigned long)sequence_number
        );

        ESP_LOGI(
            TAG,
            "GPS status    : %s",
            gps_valid
                ? "VALID"
                : "NO FIX"
        );

        if (gps_valid)
        {
            ESP_LOGI(
                TAG,
                "Latitude      : %.7f",
                gps_fix.latitude
            );

            ESP_LOGI(
                TAG,
                "Longitude     : %.7f",
                gps_fix.longitude
            );

            ESP_LOGI(
                TAG,
                "Satellites    : %u",
                satellites
            );

            ESP_LOGI(
                TAG,
                "HDOP          : %.2f",
                gps_fix.hdop
            );
        }

        ESP_LOGI(
            TAG,
            "Packet         : %s",
            packet_text
        );

        ESP_LOGI(
            TAG,
            "Packet length  : %u bytes",
            (unsigned int)actual_length
        );

        if (
            actual_length !=
            TEST_PACKET_LENGTH
        )
        {
            ESP_LOGW(
                TAG,
                "Unexpected packet length"
            );
        }

        bool sent =
            lora_send_fixed_packet(
                packet_text
            );

        ESP_LOGI(
            TAG,
            "TX result      : %s",
            sent
                ? "SUCCESS"
                : "FAILED"
        );

        printf("====================================\n");

        sequence_number++;

        vTaskDelay(
            pdMS_TO_TICKS(
                TRANSMIT_INTERVAL_MS
            )
        );
    }
}