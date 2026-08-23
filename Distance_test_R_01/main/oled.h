#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

void oled_init(void);

void oled_clear(void);

void oled_show_message(
    const char *title,
    const char *message
);

void oled_show_range_data(
    double distance_m,
    int rssi_dbm,
    float snr_db,
    double packet_loss_percent,
    uint32_t received_packets,
    uint32_t expected_packets,
    bool reliable
);

#endif
