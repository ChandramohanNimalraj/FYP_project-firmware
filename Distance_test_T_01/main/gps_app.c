#include "gps_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// =====================================================
// GPS CONFIGURATION
// =====================================================

#define GPS_UART_PORT       UART_NUM_1

#define GPS_RX_PIN          8
#define GPS_TX_PIN          9

#define GPS_BAUD_RATE       9600

#define GPS_BUFFER_SIZE     1024

static const char *TAG = "GPS";

// =====================================================
// LATEST GPS FIX
// =====================================================

static gps_fix_t latest_fix = {0};

static portMUX_TYPE gps_mutex =
    portMUX_INITIALIZER_UNLOCKED;

// =====================================================
// NMEA TO DECIMAL DEGREES
// =====================================================

static double nmea_to_decimal(
    const char *coordinate,
    char direction
)
{
    if (
        coordinate == NULL ||
        coordinate[0] == '\0'
    )
    {
        return 0.0;
    }

    double raw_value =
        strtod(coordinate, NULL);

    int degrees =
        (int)(raw_value / 100.0);

    double minutes =
        raw_value -
        ((double)degrees * 100.0);

    double decimal =
        (double)degrees +
        (minutes / 60.0);

    if (
        direction == 'S' ||
        direction == 'W'
    )
    {
        decimal = -decimal;
    }

    return decimal;
}

// =====================================================
// SPLIT NMEA FIELDS
// =====================================================

static int split_nmea(
    char *sentence,
    char *fields[],
    int maximum_fields
)
{
    int field_count = 0;

    char *field_start = sentence;

    while (field_count < maximum_fields)
    {
        fields[field_count++] =
            field_start;

        char *comma =
            strchr(field_start, ',');

        if (comma == NULL)
        {
            break;
        }

        *comma = '\0';

        field_start =
            comma + 1;
    }

    return field_count;
}

// =====================================================
// PARSE GGA SENTENCE
// =====================================================

static void parse_gga_sentence(
    char *sentence
)
{
    char sentence_copy[160];

    strncpy(
        sentence_copy,
        sentence,
        sizeof(sentence_copy) - 1
    );

    sentence_copy[
        sizeof(sentence_copy) - 1
    ] = '\0';

    // Remove checksum section
    char *checksum =
        strchr(sentence_copy, '*');

    if (checksum != NULL)
    {
        *checksum = '\0';
    }

    char *fields[20] = {0};

    int field_count =
        split_nmea(
            sentence_copy,
            fields,
            20
        );

    if (field_count < 10)
    {
        return;
    }

    /*
     * GGA fields:
     *
     * 0 = sentence type
     * 1 = UTC time
     * 2 = latitude
     * 3 = N/S
     * 4 = longitude
     * 5 = E/W
     * 6 = fix quality
     * 7 = satellites
     * 8 = HDOP
     * 9 = altitude
     */

    int fix_quality =
        atoi(fields[6]);

    gps_fix_t new_fix = {0};

    new_fix.valid =
        fix_quality > 0 &&
        fields[2][0] != '\0' &&
        fields[4][0] != '\0';

    new_fix.latitude =
        nmea_to_decimal(
            fields[2],
            fields[3][0]
        );

    new_fix.longitude =
        nmea_to_decimal(
            fields[4],
            fields[5][0]
        );

    new_fix.utc_hhmmss =
        (uint32_t)strtoul(
            fields[1],
            NULL,
            10
        );

    new_fix.satellites =
        (uint8_t)atoi(fields[7]);

    new_fix.hdop =
        strtod(fields[8], NULL);

    new_fix.altitude_m =
        strtod(fields[9], NULL);

    taskENTER_CRITICAL(&gps_mutex);

    latest_fix = new_fix;

    taskEXIT_CRITICAL(&gps_mutex);

    if (new_fix.valid)
    {
        ESP_LOGI(
            TAG,
            "FIX: Lat=%.7f Lon=%.7f Sat=%u HDOP=%.2f",
            new_fix.latitude,
            new_fix.longitude,
            new_fix.satellites,
            new_fix.hdop
        );
    }
    else
    {
        ESP_LOGW(
            TAG,
            "Waiting for GPS fix"
        );
    }
}

// =====================================================
// PROCESS NMEA SENTENCE
// =====================================================

static void process_nmea_sentence(
    char *sentence
)
{
    if (
        strncmp(
            sentence,
            "$GPGGA",
            6
        ) == 0
        ||
        strncmp(
            sentence,
            "$GNGGA",
            6
        ) == 0
    )
    {
        parse_gga_sentence(sentence);
    }
}

// =====================================================
// GPS TASK
// =====================================================

static void gps_task(
    void *parameter
)
{
    (void)parameter;

    uint8_t uart_data[256];

    char nmea_sentence[160];

    int sentence_index = 0;

    while (1)
    {
        int received_length =
            uart_read_bytes(
                GPS_UART_PORT,
                uart_data,
                sizeof(uart_data),
                pdMS_TO_TICKS(1000)
            );

        for (
            int i = 0;
            i < received_length;
            i++
        )
        {
            char character =
                (char)uart_data[i];

            if (character == '$')
            {
                sentence_index = 0;

                nmea_sentence[
                    sentence_index++
                ] = character;
            }
            else if (
                (
                    character == '\r' ||
                    character == '\n'
                )
                &&
                sentence_index > 0
            )
            {
                nmea_sentence[
                    sentence_index
                ] = '\0';

                process_nmea_sentence(
                    nmea_sentence
                );

                sentence_index = 0;
            }
            else if (
                sentence_index > 0
                &&
                sentence_index <
                sizeof(nmea_sentence) - 1
            )
            {
                nmea_sentence[
                    sentence_index++
                ] = character;
            }
        }
    }
}

// =====================================================
// GPS INITIALIZATION
// =====================================================

void gps_app_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl =
            UART_HW_FLOWCTRL_DISABLE,
        .source_clk =
            UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(
        uart_driver_install(
            GPS_UART_PORT,
            GPS_BUFFER_SIZE * 2,
            0,
            0,
            NULL,
            0
        )
    );

    ESP_ERROR_CHECK(
        uart_param_config(
            GPS_UART_PORT,
            &uart_config
        )
    );

    ESP_ERROR_CHECK(
        uart_set_pin(
            GPS_UART_PORT,
            GPS_TX_PIN,
            GPS_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );

    xTaskCreate(
        gps_task,
        "gps_task",
        4096,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(
        TAG,
        "GPS UART initialized"
    );
}

// =====================================================
// GET LATEST FIX
// =====================================================

bool gps_get_latest_fix(
    gps_fix_t *fix
)
{
    if (fix == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL(&gps_mutex);

    *fix = latest_fix;

    taskEXIT_CRITICAL(&gps_mutex);

    return fix->valid;
}