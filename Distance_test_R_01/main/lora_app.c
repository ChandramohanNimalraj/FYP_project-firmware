#include "lora_app.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lora.h"

// =====================================================
// SX127X REGISTER DEFINITIONS
// =====================================================

// Packet SNR register
#define SX127X_REG_PKT_SNR_VALUE    0x19

// Maximum expected LoRa frame:
//
// 4-byte RadioHead header
// + 36-byte sensor payload
// = 40 bytes
#define EXPECTED_FRAME_LENGTH       40

// =====================================================
// LORA INITIALIZATION
// =====================================================

void lora_app_init(void)
{
    printf("\n");
    printf("====================================\n");
    printf(" INITIALIZING LORA GATEWAY\n");
    printf("====================================\n");

    // Initialize SPI communication
    spi_init();

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    // Initialize RFM95 radio
    radio_init();

    // Use explicit LoRa header mode
    setExplicitHeaderMode();

    // Set frequency to 915 MHz
    setFrequency(915);

    // Set receiver mode
    setRxMode();

    printf("LoRa frequency : 915 MHz\n");
    printf("Header mode    : Explicit\n");
    printf("Expected frame : %d bytes\n",
           EXPECTED_FRAME_LENGTH);
    printf("Gateway RX ready\n");
}

// =====================================================
// RECEIVE LORA PACKET
// =====================================================

int lora_receive_packet(
    uint8_t *buffer,
    uint8_t buffer_capacity
)
{
    if (
        buffer == NULL ||
        buffer_capacity == 0
    )
    {
        return -1;
    }

    // Read LoRa interrupt flags
    uint8_t irq_flags =
        register_read(
            RFM9X_12_REG_IRQ_FLAGS
        );

    // No completed packet available
    if (
        (irq_flags & IRQ_RX_DONE_MASK) == 0
    )
    {
        return 0;
    }

    // Check CRC error, if supported by the library
#ifdef IRQ_PAYLOAD_CRC_ERROR_MASK

    if (
        irq_flags &
        IRQ_PAYLOAD_CRC_ERROR_MASK
    )
    {
        printf(
            "LoRa hardware CRC error\n"
        );

        register_write(
            RFM9X_12_REG_IRQ_FLAGS,
            0xFF
        );

        setRxMode();

        return -2;
    }

#endif

    // Read number of bytes received
    int packet_length =
        register_read(
            RFM9X_13_REG_RX_NB_BYTES
        );

    if (
        packet_length <= 0 ||
        packet_length > buffer_capacity
    )
    {
        printf(
            "Invalid LoRa packet length: %d\n",
            packet_length
        );

        // Clear all interrupt flags
        register_write(
            RFM9X_12_REG_IRQ_FLAGS,
            0xFF
        );

        setRxMode();

        return -2;
    }

    // Clear user receive buffer
    memset(
        buffer,
        0,
        buffer_capacity
    );

    // Get FIFO start address of current packet
    uint8_t fifo_start =
        register_read(
            RFM9X_10_REG_FIFO_RX_CURRENT_ADDR
        );

    // Point FIFO address pointer to received packet
    register_write(
        RFM9X_0D_REG_FIFO_ADDR_PTR,
        fifo_start
    );

    // Read received frame from FIFO
    for (
        int i = 0;
        i < packet_length;
        i++
    )
    {
        buffer[i] =
            register_read(
                RFM9X_00_REG_FIFO
            );
    }

    // Clear all LoRa interrupt flags
    register_write(
        RFM9X_12_REG_IRQ_FLAGS,
        0xFF
    );

    // Return radio to continuous receive mode
    setRxMode();

    printf("\n");
    printf("====================================\n");
    printf(" LORA FRAME RECEIVED\n");
    printf("====================================\n");

    printf(
        "Received frame length: %d bytes\n",
        packet_length
    );

    printf("Raw frame: ");

    for (
        int i = 0;
        i < packet_length;
        i++
    )
    {
        printf(
            "%02X ",
            buffer[i]
        );
    }

    printf("\n");

    if (
        packet_length ==
        EXPECTED_FRAME_LENGTH
    )
    {
        printf(
            "Frame size status: CORRECT\n"
        );
    }
    else
    {
        printf(
            "Frame size status: UNEXPECTED\n"
        );
    }

    printf("====================================\n");

    return packet_length;
}

// =====================================================
// GET PACKET RSSI
// =====================================================

int lora_get_packet_rssi(void)
{
    /*
     * Uses the RSSI function already available
     * in your current LoRa component.
     */
    return getPacketRSSI();
}

// =====================================================
// GET PACKET SNR
// =====================================================

float lora_get_packet_snr(void)
{
    /*
     * SX127x packet SNR is stored as a signed
     * two's-complement value in 0.25 dB steps.
     */

    int8_t raw_snr =
        (int8_t)register_read(
            SX127X_REG_PKT_SNR_VALUE
        );

    return ((float)raw_snr) / 4.0f;
}