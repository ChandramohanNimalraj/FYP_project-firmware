#include "modem.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

// ================= UART CONFIGURATION =================

#define MODEM_UART             UART_NUM_1

#define MODEM_TX_PIN           3
#define MODEM_RX_PIN           4

#define MODEM_BAUD_RATE        9600
#define MODEM_RX_BUFFER_SIZE   2048

#define MODEM_PHONE_NUMBER     "+94705744771"

static const char *TAG = "GSM_MODEM";

// Static response buffer reduces main-task stack usage
static char modem_response[512];

// ================= UART INITIALIZATION =================

static bool modem_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = MODEM_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    esp_err_t result = uart_driver_install(
        MODEM_UART,
        MODEM_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0
    );

    if (
        result != ESP_OK &&
        result != ESP_ERR_INVALID_STATE
    ) {
        ESP_LOGE(
            TAG,
            "UART driver installation failed"
        );

        return false;
    }

    result = uart_param_config(
        MODEM_UART,
        &uart_config
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "UART configuration failed"
        );

        return false;
    }

    result = uart_set_pin(
        MODEM_UART,
        MODEM_TX_PIN,
        MODEM_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "UART pin configuration failed"
        );

        return false;
    }

    uart_flush_input(
        MODEM_UART
    );

    return true;
}

// ================= CANCEL PENDING SMS =================

static void modem_cancel_pending_sms(void)
{
    const uint8_t escape_character = 0x1B;

    uart_write_bytes(
        MODEM_UART,
        (const char *)&escape_character,
        1
    );

    vTaskDelay(
        pdMS_TO_TICKS(300)
    );

    uart_flush_input(
        MODEM_UART
    );
}

// ================= READ MODEM RESPONSE =================

static int modem_read_response(
    char *response,
    size_t response_size,
    int timeout_ms
)
{
    if (
        response == NULL ||
        response_size < 2
    ) {
        return 0;
    }

    memset(
        response,
        0,
        response_size
    );

    int total_length = 0;

    int64_t start_time =
        esp_timer_get_time();

    while (
        esp_timer_get_time() - start_time <
        ((int64_t)timeout_ms * 1000)
    ) {
        size_t remaining_space =
            response_size -
            (size_t)total_length -
            1;

        if (remaining_space == 0) {
            break;
        }

        int received_length =
            uart_read_bytes(
                MODEM_UART,
                (uint8_t *)&response[total_length],
                remaining_space,
                pdMS_TO_TICKS(100)
            );

        if (received_length > 0) {
            total_length += received_length;

            response[total_length] = '\0';

            if (
                strstr(response, "\r\nOK\r\n") != NULL ||
                strstr(response, "\r\nERROR\r\n") != NULL ||
                strstr(response, "+CME ERROR") != NULL ||
                strstr(response, "+CMS ERROR") != NULL
            ) {
                break;
            }
        }
    }

    if (total_length > 0) {
        ESP_LOGI(
            TAG,
            "Response:\n%s",
            response
        );
    }

    return total_length;
}

// ================= SEND AT COMMAND =================

static bool modem_send_command(
    const char *command,
    const char *expected_response,
    int timeout_ms
)
{
    if (
        command == NULL ||
        expected_response == NULL
    ) {
        return false;
    }

    uart_flush_input(
        MODEM_UART
    );

    uart_write_bytes(
        MODEM_UART,
        command,
        strlen(command)
    );

    int response_length =
        modem_read_response(
            modem_response,
            sizeof(modem_response),
            timeout_ms
        );

    if (response_length <= 0) {
        ESP_LOGE(
            TAG,
            "No response for command: %s",
            command
        );

        return false;
    }

    if (
        strstr(
            modem_response,
            expected_response
        ) != NULL
    ) {
        return true;
    }

    ESP_LOGE(
        TAG,
        "Expected response not found: %s",
        expected_response
    );

    return false;
}

// ================= WAIT FOR SMS PROMPT =================

static bool modem_wait_for_sms_prompt(
    int timeout_ms
)
{
    memset(
        modem_response,
        0,
        sizeof(modem_response)
    );

    int total_length = 0;

    int64_t start_time =
        esp_timer_get_time();

    while (
        esp_timer_get_time() - start_time <
        ((int64_t)timeout_ms * 1000)
    ) {
        size_t remaining_space =
            sizeof(modem_response) -
            (size_t)total_length -
            1;

        if (remaining_space == 0) {
            break;
        }

        int received_length =
            uart_read_bytes(
                MODEM_UART,
                (uint8_t *)&modem_response[total_length],
                remaining_space,
                pdMS_TO_TICKS(100)
            );

        if (received_length > 0) {
            total_length += received_length;

            modem_response[total_length] = '\0';

            if (
                strchr(
                    modem_response,
                    '>'
                ) != NULL
            ) {
                ESP_LOGI(
                    TAG,
                    "SMS input prompt received"
                );

                return true;
            }

            if (
                strstr(
                    modem_response,
                    "ERROR"
                ) != NULL
            ) {
                ESP_LOGE(
                    TAG,
                    "SMS prompt error: %s",
                    modem_response
                );

                return false;
            }
        }
    }

    ESP_LOGE(
        TAG,
        "SMS input prompt not received"
    );

    return false;
}

// ================= MODEM INITIALIZATION =================

bool modem_init(void)
{
    if (!modem_uart_init()) {
        return false;
    }

    ESP_LOGI(
        TAG,
        "Waiting for GSM modem startup at 9600 baud"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );

    /*
     * Cancel an unfinished SMS command if the
     * ESP32 restarted while the modem was at ">".
     */
    modem_cancel_pending_sms();

    bool modem_ready = false;

    for (
        int attempt = 1;
        attempt <= 5;
        attempt++
    ) {
        ESP_LOGI(
            TAG,
            "AT test attempt %d",
            attempt
        );

        if (
            modem_send_command(
                "AT\r\n",
                "OK",
                2000
            )
        ) {
            modem_ready = true;
            break;
        }

        modem_cancel_pending_sms();

        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }

    if (!modem_ready) {
        ESP_LOGE(
            TAG,
            "GSM modem did not respond"
        );

        return false;
    }

    // Disable command echo
    modem_send_command(
        "ATE0\r\n",
        "OK",
        2000
    );

    // Enable SMS text mode
    if (
        !modem_send_command(
            "AT+CMGF=1\r\n",
            "OK",
            2000
        )
    ) {
        ESP_LOGE(
            TAG,
            "Unable to enable SMS text mode"
        );

        return false;
    }

    /*
     * Enable network-provided date and time.
     * AT&W saves the setting in the modem.
     */
    modem_send_command(
        "AT+CLTS=1\r\n",
        "OK",
        2000
    );

    modem_send_command(
        "AT&W\r\n",
        "OK",
        2000
    );

    ESP_LOGI(
        TAG,
        "GSM modem initialized successfully"
    );

    return true;
}

// ================= SIGNAL STRENGTH =================

bool modem_get_signal_strength(
    int *csq,
    int *rssi_dbm
)
{
    if (
        csq == NULL ||
        rssi_dbm == NULL
    ) {
        return false;
    }

    bool command_ok =
        modem_send_command(
            "AT+CSQ\r\n",
            "OK",
            2000
        );

    if (!command_ok) {
        return false;
    }

    char *csq_start =
        strstr(
            modem_response,
            "+CSQ:"
        );

    if (csq_start == NULL) {
        ESP_LOGE(
            TAG,
            "CSQ response not found"
        );

        return false;
    }

    int csq_value = 99;
    int ber_value = 99;

    int parsed = sscanf(
        csq_start,
        "+CSQ: %d,%d",
        &csq_value,
        &ber_value
    );

    if (parsed < 1) {
        ESP_LOGE(
            TAG,
            "Unable to parse CSQ response"
        );

        return false;
    }

    *csq = csq_value;

    if (csq_value == 99) {
        *rssi_dbm = 0;
    } else {
        *rssi_dbm =
            -113 +
            (2 * csq_value);
    }

    ESP_LOGI(
        TAG,
        "CSQ=%d RSSI=%d dBm BER=%d",
        *csq,
        *rssi_dbm,
        ber_value
    );

    return true;
}

// ================= NETWORK REGISTRATION =================

bool modem_is_registered(void)
{
    bool command_ok =
        modem_send_command(
            "AT+CREG?\r\n",
            "OK",
            2000
        );

    if (!command_ok) {
        return false;
    }

    char *registration_start =
        strstr(
            modem_response,
            "+CREG:"
        );

    if (registration_start == NULL) {
        ESP_LOGE(
            TAG,
            "CREG response not found"
        );

        return false;
    }

    int mode = 0;
    int status = 0;

    int parsed = sscanf(
        registration_start,
        "+CREG: %d,%d",
        &mode,
        &status
    );

    if (parsed != 2) {
        ESP_LOGE(
            TAG,
            "Unable to parse CREG response"
        );

        return false;
    }

    /*
     * Status 1 = registered on home network
     * Status 5 = registered while roaming
     */
    return (
        status == 1 ||
        status == 5
    );
}

// ================= SEND SMS =================

bool modem_send_sms(
    const char *message
)
{
    if (message == NULL) {
        return false;
    }

    if (
        !modem_send_command(
            "AT+CMGF=1\r\n",
            "OK",
            2000
        )
    ) {
        ESP_LOGE(
            TAG,
            "Unable to set SMS text mode"
        );

        return false;
    }

    char sms_command[64];

    snprintf(
        sms_command,
        sizeof(sms_command),
        "AT+CMGS=\"%s\"\r\n",
        MODEM_PHONE_NUMBER
    );

    ESP_LOGI(
        TAG,
        "Sending SMS to %s",
        MODEM_PHONE_NUMBER
    );

    uart_flush_input(
        MODEM_UART
    );

    uart_write_bytes(
        MODEM_UART,
        sms_command,
        strlen(sms_command)
    );

    if (
        !modem_wait_for_sms_prompt(
            5000
        )
    ) {
        modem_cancel_pending_sms();
        return false;
    }

    uart_write_bytes(
        MODEM_UART,
        message,
        strlen(message)
    );

    // CTRL+Z finishes and sends the SMS
    const uint8_t ctrl_z = 0x1A;

    uart_write_bytes(
        MODEM_UART,
        (const char *)&ctrl_z,
        1
    );

    int response_length =
        modem_read_response(
            modem_response,
            sizeof(modem_response),
            15000
        );

    if (response_length <= 0) {
        ESP_LOGE(
            TAG,
            "No final SMS response"
        );

        return false;
    }

    if (
        strstr(
            modem_response,
            "+CMGS:"
        ) != NULL &&
        strstr(
            modem_response,
            "OK"
        ) != NULL
    ) {
        ESP_LOGI(
            TAG,
            "SMS sent successfully"
        );

        return true;
    }

    ESP_LOGE(
        TAG,
        "SMS sending failed"
    );

    return false;
}

// ================= GSM NETWORK TIME =================

bool modem_get_network_time(
    struct tm *time_info
)
{
    if (time_info == NULL) {
        ESP_LOGE(
            TAG,
            "Invalid time information pointer"
        );

        return false;
    }

    /*
     * Request the modem's current clock.
     */
    bool command_ok =
        modem_send_command(
            "AT+CCLK?\r\n",
            "OK",
            3000
        );

    if (!command_ok) {
        ESP_LOGE(
            TAG,
            "Unable to read GSM network time"
        );

        return false;
    }

    char *clock_start =
        strstr(
            modem_response,
            "+CCLK:"
        );

    if (clock_start == NULL) {
        ESP_LOGE(
            TAG,
            "CCLK response not found"
        );

        return false;
    }

    int year = 0;
    int month = 0;
    int day = 0;

    int hour = 0;
    int minute = 0;
    int second = 0;

    char timezone_sign = '+';
    int timezone_quarters = 0;

    int parsed = sscanf(
        clock_start,
        "+CCLK: \"%2d/%2d/%2d,%2d:%2d:%2d%c%2d\"",
        &year,
        &month,
        &day,
        &hour,
        &minute,
        &second,
        &timezone_sign,
        &timezone_quarters
    );

    if (parsed < 6) {
        ESP_LOGE(
            TAG,
            "Unable to parse GSM time: %s",
            clock_start
        );

        return false;
    }

    /*
     * Reject the common invalid/default modem date.
     */
    if (
        year < 20 ||
        month < 1 ||
        month > 12 ||
        day < 1 ||
        day > 31 ||
        hour < 0 ||
        hour > 23 ||
        minute < 0 ||
        minute > 59 ||
        second < 0 ||
        second > 59
    ) {
        ESP_LOGE(
            TAG,
            "Invalid GSM network time received"
        );

        return false;
    }

    memset(
        time_info,
        0,
        sizeof(struct tm)
    );

    /*
     * struct tm counts years from 1900.
     * Example: modem year 26 means 2026.
     */
    time_info->tm_year =
        year + 100;

    // struct tm months use 0–11
    time_info->tm_mon =
        month - 1;

    time_info->tm_mday =
        day;

    time_info->tm_hour =
        hour;

    time_info->tm_min =
        minute;

    time_info->tm_sec =
        second;

    time_info->tm_isdst =
        -1;

    ESP_LOGI(
        TAG,
        "GSM time: 20%02d-%02d-%02d "
        "%02d:%02d:%02d %c%02d",
        year,
        month,
        day,
        hour,
        minute,
        second,
        timezone_sign,
        timezone_quarters
    );

    return true;
}