#ifndef LORA_APP_H
#define LORA_APP_H

#include <stdint.h>

// Initialize the RFM95 LoRa module
void lora_app_init(void);

// Receive the complete LoRa frame
//
// Return values:
//   > 0  = received frame length
//     0  = no packet available
//    -1  = invalid argument
//    -2  = invalid received packet length
int lora_receive_packet(
    uint8_t *buffer,
    uint8_t buffer_capacity
);

// Get RSSI of the most recently received packet
int lora_get_packet_rssi(void);

// Get SNR of the most recently received packet
float lora_get_packet_snr(void);

#endif