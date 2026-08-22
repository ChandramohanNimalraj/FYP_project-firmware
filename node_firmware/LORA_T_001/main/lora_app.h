#ifndef LORA_APP_H
#define LORA_APP_H

#include <stdbool.h>
#include <stdint.h>

// ================= INIT =================
void lora_app_init(void);

// ================= SEND RAW PACKET =================
bool lora_send_packet(
    uint8_t *buffer,
    uint8_t length
);

// ================= RECEIVE RAW PACKET =================
int lora_receive_packet(
    uint8_t *buffer
);

#endif