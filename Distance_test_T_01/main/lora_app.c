#include "lora_app.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lora.h"

// =====================================================
// LORA INITIALIZATION
// =====================================================

void lora_app_init(void)
{
    printf("\n");
    printf("=============================\n");
    printf(" INITIALIZING LORA\n");
    printf("=============================\n");

    spi_init();

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    radio_init();

    setExplicitHeaderMode();

    setFrequency(433);

    setTxPower(17);

    setRxMode();

    printf("LoRa frequency : 433 MHz\n");
    printf("TX power       : 17 dBm\n");
    printf("Header mode    : Explicit\n");
    printf("LoRa ready\n");
}

// =====================================================
// SEND FIXED ASCII PACKET
// =====================================================

bool lora_send_fixed_packet(
    const char *packet_text
)
{
    if (packet_text == NULL)
    {
        return false;
    }

    size_t packet_length =
        strlen(packet_text);

    if (packet_length == 0)
    {
        return false;
    }

    printf("\n");
    printf("=============================\n");
    printf(" SENDING LORA PACKET\n");
    printf("=============================\n");

    printf(
        "Packet length: %u bytes\n",
        (unsigned int)packet_length
    );

    printf(
        "Packet data  : %s\n",
        packet_text
    );

    send(
        (char *)packet_text
    );

    printf("Packet sent successfully\n");

    setRxMode();

    return true;
}