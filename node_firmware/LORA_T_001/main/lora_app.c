#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lora.h"

#include "lora_app.h"

// ================= LORA INIT =================
void lora_app_init(void)
{
    printf("Initializing LoRa...\n");

    // SPI Init
    spi_init();

    vTaskDelay(pdMS_TO_TICKS(100));

    // Radio Init
    radio_init();
    
    setExplicitHeaderMode();

    // Frequency
    setFrequency(433);

    // TX Power
    setTxPower(17);

    // RX Mode
    setRxMode();

    printf("LoRa Initialized Successfully\n");
}

// ================= SEND PACKET =================
bool lora_send_packet(
    uint8_t *buffer,
    uint8_t length
)
{
    if (buffer == NULL)
    {
        return false;
    }

    if (length == 0)
    {
        return false;
    }

    printf("\n");
    printf("==============================\n");
    printf("Sending LoRa Packet\n");
    printf("Packet Length : %d bytes\n",
           length);
    printf("==============================\n");

    // Send raw packet
    send((char *)buffer);

    printf("Packet Sent Successfully\n");

    return true;
}

// ================= RECEIVE PACKET =================
int lora_receive_packet(
    uint8_t *buffer
)
{
    if (buffer == NULL)
    {
        return -1;
    }

    // Set RX mode
    setRxMode();

    uint8_t irq_flags =
        register_read(
            RFM9X_12_REG_IRQ_FLAGS
        );

    // Packet received
    if (irq_flags & IRQ_RX_DONE_MASK)
    {
        int length =
            register_read(
                RFM9X_13_REG_RX_NB_BYTES
            );

        if (length > 0)
        {
            // Get FIFO start address
            uint8_t fifoStart =
                register_read(
                    RFM9X_10_REG_FIFO_RX_CURRENT_ADDR
                );

            register_write(
                RFM9X_0D_REG_FIFO_ADDR_PTR,
                fifoStart
            );

            // Read packet bytes
            for (int i = 0; i < length; i++)
            {
                buffer[i] =
                    register_read(
                        RFM9X_00_REG_FIFO
                    );
            }

            // Clear IRQ flags
            register_write(
                RFM9X_12_REG_IRQ_FLAGS,
                0xFF
            );

            printf("\n");
            printf("==============================\n");
            printf("LoRa Packet Received\n");
            printf("Packet Length : %d bytes\n",
                   length);
            printf("==============================\n");

            return length;
        }
    }

    return -1;
}