#include "modem.h"

#include <stdio.h>
#include <string.h>

#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ================= UART CONFIG =================
#define MODEM_UART UART_NUM_1

// CHANGE THESE PINS IF NEEDED
#define MODEM_TX_PIN 4
#define MODEM_RX_PIN 3

#define BUF_SIZE 1024

// ================= SEND AT COMMAND =================
static void modem_send_cmd(const char *cmd)
{
    uart_write_bytes(
        MODEM_UART,
        cmd,
        strlen(cmd)
    );
}

// ================= READ RESPONSE =================
static void modem_read_response(void)
{
    uint8_t data[BUF_SIZE];

    int len = uart_read_bytes(
        MODEM_UART,
        data,
        BUF_SIZE - 1,
        pdMS_TO_TICKS(1000)
    );

    if (len > 0)
    {
        data[len] = '\0';

        printf("%s\n", data);
    }
}

// ================= MODEM INIT =================
void modem_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(
        MODEM_UART,
        BUF_SIZE * 2,
        0,
        0,
        NULL,
        0
    );

    uart_param_config(
        MODEM_UART,
        &uart_config
    );

    uart_set_pin(
        MODEM_UART,
        MODEM_TX_PIN,
        MODEM_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );

    printf("Initializing GSM Modem...\n");

    vTaskDelay(pdMS_TO_TICKS(3000));

    // Basic AT test
    modem_send_cmd("AT\r\n");

    vTaskDelay(pdMS_TO_TICKS(1000));

    modem_read_response();

    // SMS text mode
    modem_send_cmd("AT+CMGF=1\r\n");

    vTaskDelay(pdMS_TO_TICKS(1000));

    modem_read_response();

    printf("MODEM READY\n");
}

// ================= SEND SMS =================
void modem_send_sms(const char *message)
{
    char cmd[64];

    printf("Sending SMS...\n");

    sprintf(
        cmd,
        "AT+CMGS=\"+94770501623\"\r\n"
    );

    modem_send_cmd(cmd);

    vTaskDelay(pdMS_TO_TICKS(2000));

    modem_read_response();

    // Send message text
    modem_send_cmd(message);

    // CTRL+Z to send SMS
    uart_write_bytes(
        MODEM_UART,
        "\x1A",
        1
    );

    vTaskDelay(pdMS_TO_TICKS(5000));

    modem_read_response();

    printf("SMS SENT\n");
}