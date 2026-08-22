#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hx711_app.h"

#include "lora_app.h"

#include "protocol.h"

// ================= NODE CONFIG =================
#define MY_NODE_ID    NODE_1_ID

// ================= MAIN =================
void app_main(void)
{
    printf("\n");
    printf("====================================\n");
    printf("  LoRa Weight Transmitter Start\n");
    printf("====================================\n");

    // ================= HX711 INIT =================
    hx711_app_init();

    // ================= LORA INIT =================
    lora_app_init();

    // ================= VARIABLES =================
    char payload[16];

    // Sensor packet
    sensor_packet_t sensor_packet;

    // Packet sequence number
    uint8_t sequence = 0;

    // ================= MAIN LOOP =================
    while (1)
    {
        // ================= READ WEIGHT =================
        float weight =
            hx711_get_weight();

        // ================= CREATE PAYLOAD =================
        sprintf(
            payload,
            "%.2f",
            weight
        );

        printf("\n");
        printf("====================================\n");
        printf("Weight Payload : %s g\n",
               payload);
        printf("Sequence No    : %d\n",
               sequence);
        printf("====================================\n");

        // ================= CREATE SENSOR PACKET =================
        protocol_create_sensor_packet(
            &sensor_packet,
            MY_NODE_ID,
            GATEWAY_ID,
            sequence,
            payload
        );

        // ================= SEND PACKET =================
        lora_send_packet(
            sensor_packet.buffer,
            SENSOR_PACKET_LENGTH
        );

        printf("LoRa Packet Sent\n");

        // ================= PRINT RAW PACKET =================
        printf("Packet Bytes : ");

        for (int i = 0;
             i < SENSOR_PACKET_LENGTH;
             i++)
        {
            printf("%02X ",
                   sensor_packet.buffer[i]);
        }

        printf("\n");

        // ================= NEXT SEQUENCE =================
        sequence++;

        // ================= SEND INTERVAL =================
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}