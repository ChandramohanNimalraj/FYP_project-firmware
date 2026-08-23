#ifndef MODEM_H
#define MODEM_H

#include <stdbool.h>
#include <time.h>

bool modem_init(void);

bool modem_get_signal_strength(
    int *csq,
    int *rssi_dbm
);

bool modem_is_registered(void);

bool modem_send_sms(
    const char *message
);

bool modem_get_network_time(
    struct tm *time_info
);

#endif