#ifndef LORA_APP_H
#define LORA_APP_H

#include <stdbool.h>

void lora_app_init(void);

bool lora_send_fixed_packet(
    const char *packet_text
);

#endif