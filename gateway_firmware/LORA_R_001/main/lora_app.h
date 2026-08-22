#ifndef LORA_APP_H
#define LORA_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "lora.h"
// ================= INIT =================
void lora_app_init(void);

// ================= RECEIVE PACKET =================
int lora_receive_packet(
    uint8_t *buffer
);

// ================= SEND PACKET =================
bool lora_send_packet(
    uint8_t *buffer,
    uint8_t length
);

#endif