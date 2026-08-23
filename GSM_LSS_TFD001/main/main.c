#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "modem.h"
#include "oled.h"
#include "encoder.h"

// ================= CONFIGURATION =================

#define SIGNAL_CHECK_INTERVAL_MS   5000
#define DISPLAY_UPDATE_INTERVAL_MS 1000
#define TIME_SYNC_RETRY_MS         10000
#define TIME_RESYNC_INTERVAL_MS    3600000

#define SMS_STATUS_DELAY_MS        2500
#define SMS_COOLDOWN_MS            3000

static const char *TAG = "GSM_TEST";

// ================= SYSTEM DATA =================

static int current_csq = 99;
static int current_rssi_dbm = 0;

static bool current_registered = false;
static bool system_time_valid = false;

// ================= SIGNAL QUALITY =================

static const char *get_signal_quality(int csq)
{
    if (csq == 99) {
        return "UNKNOWN";
    }

    if (csq <= 9) {
        return "MARGINAL";
    }

    if (csq <= 14) {
        return "ACCEPTABLE";
    }

    if (csq <= 19) {
        return "GOOD";
    }

    return "EXCELLENT";
}

// ================= TIME SYNCHRONIZATION =================

static bool synchronize_time_from_gsm(void)
{
    struct tm gsm_time;

    memset(
        &gsm_time,
        0,
        sizeof(gsm_time)
    );

    ESP_LOGI(
        TAG,
        "Requesting GSM network time"
    );

    bool time_received =
        modem_get_network_time(
            &gsm_time
        );

    if (!time_received) {
        ESP_LOGE(
            TAG,
            "Unable to get GSM network time"
        );

        return false;
    }

    /*
     * The SIM900A time is treated as the local time
     * supplied by the GSM network.
     *
     * UTC0 prevents the C library from applying another
     * timezone conversion to the modem's local time.
     */
    setenv(
        "TZ",
        "UTC0",
        1
    );

    tzset();

    time_t epoch_time =
        mktime(
            &gsm_time
        );

    if (epoch_time == (time_t)-1) {
        ESP_LOGE(
            TAG,
            "Unable to convert GSM time"
        );

        return false;
    }

    struct timeval system_time = {
        .tv_sec = epoch_time,
        .tv_usec = 0
    };

    if (
        settimeofday(
            &system_time,
            NULL
        ) != 0
    ) {
        ESP_LOGE(
            TAG,
            "Unable to set ESP32 system time"
        );

        return false;
    }

    system_time_valid = true;

    ESP_LOGI(
        TAG,
        "ESP32 time synchronized: "
        "%04d-%02d-%02d %02d:%02d:%02d",
        gsm_time.tm_year + 1900,
        gsm_time.tm_mon + 1,
        gsm_time.tm_mday,
        gsm_time.tm_hour,
        gsm_time.tm_min,
        gsm_time.tm_sec
    );

    return true;
}

// ================= GET CURRENT TIME =================

static bool get_current_time(
    struct tm *time_info
)
{
    if (
        time_info == NULL ||
        !system_time_valid
    ) {
        return false;
    }

    time_t now;

    time(
        &now
    );

    localtime_r(
        &now,
        time_info
    );

    /*
     * Reject invalid/default dates.
     */
    if (
        time_info->tm_year + 1900 < 2020
    ) {
        return false;
    }

    return true;
}

// ================= OLED DISPLAY =================

static void show_system_data(void)
{
    static char display_text[160];

    struct tm time_info;

    bool time_available =
        get_current_time(
            &time_info
        );

    if (time_available) {
        if (current_csq == 99) {
            snprintf(
                display_text,
                sizeof(display_text),
                "%02d/%02d/%04d\n"
                "%02d:%02d:%02d\n"
                "CSQ: UNKNOWN\n"
                "CW: SEND SMS",
                time_info.tm_mday,
                time_info.tm_mon + 1,
                time_info.tm_year + 1900,
                time_info.tm_hour,
                time_info.tm_min,
                time_info.tm_sec
            );
        } else {
            snprintf(
                display_text,
                sizeof(display_text),
                "%02d/%02d/%04d"
                "  %02d:%02d:%02d\n"
                "\n"
                "CSQ:%d\n"
                "RSSI:%d\n"
                "         CW: SEND SMS",
                time_info.tm_mday,
                time_info.tm_mon + 1,
                time_info.tm_year + 1900,
                time_info.tm_hour,
                time_info.tm_min,
                time_info.tm_sec,
                current_csq,
                current_rssi_dbm
            );
        }
    } else {
        if (current_csq == 99) {
            snprintf(
                display_text,
                sizeof(display_text),
                "TIME: NOT SYNC\n"
                "CSQ: UNKNOWN\n"
                "NET: %s\n"
                "CW: SEND SMS",
                current_registered ?
                    "REGISTERED" :
                    "NOT REGISTERED"
            );
        } else {
            snprintf(
                display_text,
                sizeof(display_text),
                "TIME: NOT SYNC\n"
                "CSQ: %d/31\n"
                "RSSI: %d dBm\n"
                "CW: SEND SMS",
                current_csq,
                current_rssi_dbm
            );
        }
    }

    oled_show_message(
        "GSM SIGNAL TEST",
        display_text
    );
}

// ================= SMS TEST =================

static void send_test_sms(void)
{
    static char sms_message[200];

    oled_show_message(
        "SMS TEST",
        "Checking signal..."
    );

    bool signal_ok =
        modem_get_signal_strength(
            &current_csq,
            &current_rssi_dbm
        );

    current_registered =
        modem_is_registered();

    struct tm time_info;

    bool time_available =
        get_current_time(
            &time_info
        );

    if (
        signal_ok &&
        current_csq != 99 &&
        time_available
    ) {
        snprintf(
            sms_message,
            sizeof(sms_message),
            "GSM attenuation test\n"
            "Date: %02d/%02d/%04d\n"
            "Time: %02d:%02d:%02d\n"
            "CSQ: %d/31\n"
            "RSSI: %d dBm\n"
            "Quality: %s\n"
            "Network: %s",
            time_info.tm_mday,
            time_info.tm_mon + 1,
            time_info.tm_year + 1900,
            time_info.tm_hour,
            time_info.tm_min,
            time_info.tm_sec,
            current_csq,
            current_rssi_dbm,
            get_signal_quality(current_csq),
            current_registered ?
                "Registered" :
                "Not registered"
        );
    } else if (
        signal_ok &&
        current_csq != 99
    ) {
        snprintf(
            sms_message,
            sizeof(sms_message),
            "GSM attenuation test\n"
            "Time: Not synchronized\n"
            "CSQ: %d/31\n"
            "RSSI: %d dBm\n"
            "Quality: %s\n"
            "Network: %s",
            current_csq,
            current_rssi_dbm,
            get_signal_quality(current_csq),
            current_registered ?
                "Registered" :
                "Not registered"
        );
    } else {
        snprintf(
            sms_message,
            sizeof(sms_message),
            "GSM attenuation test\n"
            "Time: %s\n"
            "CSQ: Unknown\n"
            "RSSI: Unknown\n"
            "Network: %s",
            time_available ?
                "Available" :
                "Not synchronized",
            current_registered ?
                "Registered" :
                "Not registered"
        );
    }

    oled_show_message(
        "SMS TEST",
        "Sending SMS...\n"
        "Please wait"
    );

    bool sms_sent =
        modem_send_sms(
            sms_message
        );

    if (sms_sent) {
        ESP_LOGI(
            TAG,
            "SMS sent successfully"
        );

        oled_show_message(
            "SMS SUCCESS",
            "Message sent\n"
            "successfully"
        );
    } else {
        ESP_LOGE(
            TAG,
            "SMS sending failed"
        );

        oled_show_message(
            "SMS FAILED",
            "Check signal,\n"
            "SIM and network"
        );
    }

    vTaskDelay(
        pdMS_TO_TICKS(
            SMS_STATUS_DELAY_MS
        )
    );
}

// ================= MAIN =================

void app_main(void)
{
    bool modem_ready = false;

    int64_t last_signal_check = 0;
    int64_t last_display_update = 0;
    int64_t last_sms_time = 0;
    int64_t last_time_sync_attempt = 0;
    int64_t last_successful_time_sync = 0;

    ESP_LOGI(
        TAG,
        "Starting GSM signal test"
    );

    // Initialize OLED
    oled_init();

    oled_show_message(
        "GSM SIGNAL TEST",
        "Starting system..."
    );

    // Initialize rotary encoder
    encoder_init();

    vTaskDelay(
        pdMS_TO_TICKS(500)
    );

    // Initialize GSM modem
    oled_show_message(
        "GSM SIGNAL TEST",
        "Initializing modem..."
    );

    modem_ready =
        modem_init();

    while (!modem_ready) {
        ESP_LOGE(
            TAG,
            "Modem initialization failed"
        );

        oled_show_message(
            "MODEM ERROR",
            "No modem response\n"
            "Retrying..."
        );

        vTaskDelay(
            pdMS_TO_TICKS(5000)
        );

        modem_ready =
            modem_init();
    }

    oled_show_message(
        "GSM SIGNAL TEST",
        "Modem ready\n"
        "Waiting network..."
    );

    /*
     * Read network registration before synchronizing time.
     */
    for (
        int attempt = 1;
        attempt <= 12;
        attempt++
    ) {
        current_registered =
            modem_is_registered();

        if (current_registered) {
            ESP_LOGI(
                TAG,
                "GSM network registered"
            );

            break;
        }

        ESP_LOGW(
            TAG,
            "Waiting for network registration: %d/12",
            attempt
        );

        oled_show_message(
            "GSM SIGNAL TEST",
            "Waiting for\n"
            "GSM network..."
        );

        vTaskDelay(
            pdMS_TO_TICKS(3000)
        );
    }

    /*
     * Initial time synchronization.
     */
    if (current_registered) {
        oled_show_message(
            "GSM SIGNAL TEST",
            "Synchronizing\n"
            "network time..."
        );

        if (
            synchronize_time_from_gsm()
        ) {
            last_successful_time_sync =
                esp_timer_get_time();
        }
    }

    int64_t current_start_time =
        esp_timer_get_time();

    last_signal_check =
        current_start_time -
        ((int64_t)SIGNAL_CHECK_INTERVAL_MS * 1000);

    last_display_update =
        current_start_time -
        ((int64_t)DISPLAY_UPDATE_INTERVAL_MS * 1000);

    last_time_sync_attempt =
        current_start_time;

    while (true) {
        int64_t current_time_us =
            esp_timer_get_time();

        // ================= ROTARY ENCODER =================

        encoder_direction_t direction =
            encoder_get_direction();

        if (
            direction ==
            ENCODER_CLOCKWISE
        ) {
            ESP_LOGI(
                TAG,
                "Encoder clockwise"
            );

            if (
                current_time_us - last_sms_time >=
                ((int64_t)SMS_COOLDOWN_MS * 1000)
            ) {
                last_sms_time =
                    current_time_us;

                ESP_LOGI(
                    TAG,
                    "Clockwise detected - sending SMS"
                );

                send_test_sms();

                /*
                 * Refresh signal and display after SMS.
                 */
                last_signal_check = 0;
                last_display_update = 0;
            } else {
                ESP_LOGW(
                    TAG,
                    "SMS cooldown active"
                );
            }
        } else if (
            direction ==
            ENCODER_COUNTERCLOCKWISE
        ) {
            ESP_LOGI(
                TAG,
                "Encoder counterclockwise"
            );
        }

        // ================= SIGNAL CHECK =================

        if (
            current_time_us - last_signal_check >=
            ((int64_t)SIGNAL_CHECK_INTERVAL_MS * 1000)
        ) {
            last_signal_check =
                current_time_us;

            bool signal_ok =
                modem_get_signal_strength(
                    &current_csq,
                    &current_rssi_dbm
                );

            current_registered =
                modem_is_registered();

            if (signal_ok) {
                ESP_LOGI(
                    TAG,
                    "CSQ=%d RSSI=%d dBm NET=%s",
                    current_csq,
                    current_rssi_dbm,
                    current_registered ?
                        "REGISTERED" :
                        "NOT REGISTERED"
                );
            } else {
                ESP_LOGE(
                    TAG,
                    "Unable to read signal strength"
                );

                current_csq = 99;
                current_rssi_dbm = 0;
            }
        }

        // ================= TIME SYNC RETRY =================

        if (
            !system_time_valid &&
            current_registered &&
            current_time_us - last_time_sync_attempt >=
            ((int64_t)TIME_SYNC_RETRY_MS * 1000)
        ) {
            last_time_sync_attempt =
                current_time_us;

            oled_show_message(
                "GSM SIGNAL TEST",
                "Synchronizing\n"
                "network time..."
            );

            if (
                synchronize_time_from_gsm()
            ) {
                last_successful_time_sync =
                    current_time_us;
            }
        }

        // ================= PERIODIC TIME RESYNC =================

        if (
            system_time_valid &&
            current_registered &&
            current_time_us - last_successful_time_sync >=
            ((int64_t)TIME_RESYNC_INTERVAL_MS * 1000)
        ) {
            last_successful_time_sync =
                current_time_us;

            if (
                synchronize_time_from_gsm()
            ) {
                ESP_LOGI(
                    TAG,
                    "Periodic GSM time synchronization complete"
                );
            }
        }

        // ================= OLED UPDATE =================

        if (
            current_time_us - last_display_update >=
            ((int64_t)DISPLAY_UPDATE_INTERVAL_MS * 1000)
        ) {
            last_display_update =
                current_time_us;

            show_system_data();
        }

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}