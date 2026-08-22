#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "oled.h"

#include "modem.h"

#include "lora_app.h"

#include "protocol.h"

#include "data_packet.h"

// =====================================================
// MAIN
// =====================================================
void app_main(void)
{
    printf("\n");
    printf("====================================\n");
    printf("   LoRa Gateway Receiver Start\n");
    printf("====================================\n");

    // =================================================
    // OLED INIT
    // =================================================
    
    printf("OLED INIT START\n");
    oled_init();

    

    oled_show_message(
        "SYSTEM",
        "STARTING..."
    );

    // =================================================
    // MODEM INIT
    // =================================================
    modem_init();

    oled_show_message(
        "MODEM",
        "READY"
    );

    vTaskDelay(pdMS_TO_TICKS(2000));

    // =================================================
    // LORA INIT
    // =================================================
    lora_app_init();

    oled_show_message(
        "LORA",
        "RX READY"
    );

    printf("Gateway Ready\n");

    // =================================================
    // VARIABLES
    // =================================================
    uint8_t rx_buffer[64];

    data_packet_t data_packet;

    ack_packet_t ack_packet;

    int sms_sent = 0;

    float threshold_weight = 2000.0;

    // =================================================
    // MAIN LOOP
    // =================================================
    while (1)
    {

        // ==========================================
        // CLEAR BUFFERS FIRST
        // ==========================================
            memset(
                  rx_buffer,
                  0,
                  sizeof(rx_buffer)
         );

             memset(
                    data_packet.buffer,
                    0,
                    sizeof(data_packet.buffer)
          );
        // =============================================
        // RECEIVE PACKET
        // =============================================
        int length =
            lora_receive_packet(
                rx_buffer
            );

        // =============================================
        // PACKET RECEIVED
        // =============================================
        if (length > 0)
        {



            printf("\n");
            printf("====================================\n");
            printf(" RAW RX BUFFER\n");
            printf("====================================\n");

            // Print raw received bytes
            for (int i = 0; i < length; i++)
            {
                printf("RX[%02d] = 0x%02X\n",
                       i,
                       rx_buffer[i]);
            }

            printf("====================================\n");

            // =========================================
            // REMOVE RADIOHEAD HEADER
            // =========================================
            memcpy(
    data_packet.buffer,
    rx_buffer + 4,
    DATA_PACKET_LENGTH
            );

            printf("\n");
            printf("====================================\n");
            printf(" DATA PACKET PARSED\n");
            printf("====================================\n");

            // =========================================
            // PRINT PACKET BYTES
            // =========================================
            for (int i = 0;
                 i < DATA_PACKET_LENGTH;
                 i++)
            {
                printf("Packet[%02d] = 0x%02X\n",
                       i,
                       data_packet.buffer[i]);
            }

            printf("====================================\n");

            // =========================================
            // PRINT PACKET FIELDS
            // =========================================
            printf("Start Byte     : 0x%02X\n",
                   data_packet._.start_byte);

            printf("Sender ID      : %d\n",
                   data_packet._.sender_id);

            printf("Receiver ID    : %d\n",
                   data_packet._.receiver_id);

            printf("Packet Type    : %d\n",
                   data_packet._.packet_type);

            printf("Sequence No    : %d\n",
                   data_packet._.sequence);

            printf("Payload Length : %d\n",
                   data_packet._.payload_length);

            printf("Payload        : %.*s\n",
                   data_packet._.payload_length,
                   data_packet._.payload );
          
            printf("Checksum Byte  : 0x%02X\n",
                   data_packet._.checksum);

            printf("End Byte       : 0x%02X\n",
                   data_packet._.end_byte);

            printf("====================================\n");

            // =========================================
            // CHECKSUM DEBUG
            // =========================================
            uint8_t checksum_total = 0;

            for (int i = 0;
                 i < DATA_PACKET_LENGTH - 1;
                 i++)
            {
                checksum_total +=
                    data_packet.buffer[i];
            }

            printf("Checksum Total = 0x%02X\n",
                   checksum_total);

            // =========================================
            // VALIDATE CHECKSUM
            // =========================================
            if (
                protocol_validate_checksum(
                    data_packet.buffer,
                    DATA_PACKET_LENGTH
                ) == false
            )
            {
                printf("\n");
                printf("====================================\n");
                printf(" CHECKSUM FAILED\n");
                printf("====================================\n");

                oled_show_message(
                    "ERROR",
                    "CHECKSUM"
                );

                continue;
            }

            printf("\n");
            printf("====================================\n");
            printf(" CHECKSUM VALID\n");
            printf("====================================\n");

            // =========================================
            // RSSI
            // =========================================
             int rssi = getPacketRSSI();

             printf("RSSI = %d dBm\n", rssi);


            // =========================================
            // OLED DISPLAY
            // =========================================
            char oled_title[32];

            char oled_message[32];

            // =========================================
            // OLED TITLE
            // =========================================
            sprintf(
                    oled_title,
                    "MSG FROM NODE %02X",
                    data_packet._.sender_id
           );

           // =========================================
           // OLED MESSAGE
           // =========================================
           sprintf(
                   oled_message,
                   "WEIGHT:%.*s g",
                   data_packet._.payload_length,
                   data_packet._.payload
           );

            // =========================================
            // SHOW OLED
            // =========================================
            oled_show_message(
            oled_title,
            oled_message
            );

            // =========================================
            // CREATE ACK
            // =========================================
            protocol_create_ack_packet(
                &ack_packet,
                GATEWAY_ID,
                data_packet._.sender_id,
                data_packet._.sequence,
                ACK_STATUS_SUCCESS
            );

            // =========================================
            // SEND ACK
            // =========================================
            lora_send_packet(
                ack_packet.buffer,
                ACK_PACKET_LENGTH
            );

            printf("ACK SENT\n");

            // =========================================
            // WEIGHT PROCESSING
            // =========================================
            float weight =
                atof(
                    data_packet._.payload
                );

            printf("Weight = %.2f g\n",
                   weight);

            // =========================================
            // GSM ALERT
            // =========================================
            if (
                weight >= threshold_weight
                &&
                sms_sent == 0
            )
            {
                printf("\n");
                printf("====================================\n");
                printf(" WEIGHT ABOVE 2KG DETECTED\n");
                printf(" SENDING SMS ALERT\n");
                printf("====================================\n");

                oled_show_message(
                    "ALERT",
                    "SENDING SMS"
                );

                char sms_text[128];

                sprintf(
                    sms_text,
                    "ALERT: Weight exceeded 2kg. Weight = %.2f g",
                    weight
                );

                modem_send_sms(
                    sms_text
                );

                printf("SMS SENT SUCCESSFULLY\n");

                oled_show_message(
                    "SMS",
                    "SENT"
                );

                sms_sent = 1;
            }

            // =========================================
            // RESET SMS FLAG
            // =========================================
            if (weight < threshold_weight)
            {
                sms_sent = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}