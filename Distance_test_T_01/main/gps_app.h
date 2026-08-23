#ifndef GPS_APP_H
#define GPS_APP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool valid;

    double latitude;
    double longitude;
    double altitude_m;

    uint32_t utc_hhmmss;

    uint8_t satellites;

    double hdop;

} gps_fix_t;

void gps_app_init(void);

bool gps_get_latest_fix(
    gps_fix_t *fix
);

#endif