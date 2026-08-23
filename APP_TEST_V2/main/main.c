#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "modem.h"
#include "oled.h"
#include "encoder.h"

// ================= CONFIGURATION =================

#define SMS_COOLDOWN_MS      3000
#define SMS_STATUS_DELAY_MS  2500

static const char *TAG = "HIVE_SMS";

// ================= HIVE DATA =================

typedef struct {
    uint16_t day;
    float hive1;
    float hive2;
    float hive3;
} hive_weight_sample_t;

#define HIVE_WEIGHT_SAMPLE_COUNT 164

static const hive_weight_sample_t hive_weight_data[HIVE_WEIGHT_SAMPLE_COUNT] = {
    { 120, 35.955f, 39.945f, 39.680f },
    { 121, 36.075f, 40.090f, 39.750f },
    { 122, 36.185f, 40.290f, 39.835f },
    { 123, 35.750f, 40.730f, 39.800f },
    { 124, 35.455f, 41.030f, 39.705f },
    { 125, 35.565f, 41.160f, 39.640f },
    { 126, 35.660f, 41.440f, 39.675f },
    { 127, 35.640f, 41.435f, 39.745f },
    { 128, 35.645f, 41.415f, 39.790f },
    { 129, 35.675f, 41.485f, 39.845f },
    { 130, 35.680f, 41.910f, 39.805f },
    { 131, 35.790f, 42.190f, 39.820f },
    { 132, 35.895f, 42.560f, 39.900f },
    { 133, 35.875f, 42.815f, 40.125f },
    { 134, 35.945f, 42.985f, 40.265f },
    { 135, 36.310f, 43.520f, 40.445f },
    { 136, 36.950f, 44.070f, 40.720f },
    { 137, 37.285f, 44.225f, 40.730f },
    { 138, 37.180f, 44.115f, 40.650f },
    { 139, 37.080f, 44.000f, 40.570f },
    { 140, 37.140f, 44.395f, 40.695f },
    { 141, 37.875f, 45.340f, 40.920f },
    { 142, 38.695f, 45.815f, 41.115f },
    { 143, 38.990f, 45.955f, 41.155f },
    { 144, 39.710f, 46.590f, 41.435f },
    { 145, 40.410f, 47.630f, 41.825f },
    { 146, 41.350f, 48.540f, 42.260f },
    { 147, 41.805f, 49.000f, 42.565f },
    { 148, 41.845f, 49.000f, 42.625f },
    { 149, 41.940f, 48.865f, 42.710f },
    { 150, 42.150f, 48.920f, 42.875f },
    { 151, 42.475f, 49.025f, 43.120f },
    { 152, 42.730f, 49.220f, 43.435f },
    { 153, 43.160f, 49.495f, 43.915f },
    { 154, 43.470f, 49.640f, 44.405f },
    { 155, 43.580f, 49.620f, 44.570f },
    { 156, 43.610f, 49.450f, 44.540f },
    { 157, 43.785f, 49.360f, 44.625f },
    { 158, 44.065f, 49.345f, 44.825f },
    { 159, 44.475f, 49.385f, 45.040f },
    { 160, 44.865f, 49.375f, 45.185f },
    { 161, 44.960f, 49.330f, 45.310f },
    { 162, 44.995f, 49.240f, 45.400f },
    { 163, 44.820f, 49.075f, 45.440f },
    { 164, 44.695f, 49.065f, 45.420f },
    { 165, 45.100f, 49.280f, 45.600f },
    { 166, 45.415f, 49.445f, 45.825f },
    { 167, 45.685f, 49.525f, 46.060f },
    { 168, 46.205f, 49.745f, 46.395f },
    { 169, 47.250f, 50.235f, 46.880f },
    { 170, 48.210f, 51.040f, 47.650f },
    { 171, 48.935f, 51.495f, 48.470f },
    { 172, 49.260f, 51.715f, 49.330f },
    { 173, 49.010f, 51.780f, 49.800f },
    { 174, 48.720f, 51.790f, 49.855f },
    { 175, 48.810f, 51.725f, 50.105f },
    { 176, 49.115f, 51.800f, 50.280f },
    { 177, 49.350f, 51.760f, 50.375f },
    { 178, 49.475f, 51.670f, 50.490f },
    { 179, 49.285f, 51.640f, 50.755f },
    { 180, 49.250f, 51.730f, 51.065f },
    { 181, 49.520f, 51.810f, 51.390f },
    { 182, 49.660f, 51.835f, 51.555f },
    { 183, 49.205f, 51.910f, 51.625f },
    { 184, 49.680f, 52.090f, 51.975f },
    { 185, 49.635f, 52.270f, 52.335f },
    { 186, 49.490f, 52.240f, 52.305f },
    { 187, 49.350f, 52.205f, 52.035f },
    { 188, 49.305f, 52.295f, 51.975f },
    { 189, 49.225f, 52.285f, 51.825f },
    { 190, 48.980f, 52.160f, 51.560f },
    { 191, 48.840f, 52.135f, 51.465f },
    { 192, 48.805f, 52.210f, 51.370f },
    { 193, 48.740f, 52.275f, 51.310f },
    { 194, 48.735f, 52.260f, 51.395f },
    { 195, 48.670f, 52.175f, 51.305f },
    { 196, 48.575f, 52.135f, 51.390f },
    { 197, 48.340f, 52.110f, 51.655f },
    { 198, 48.155f, 52.020f, 51.640f },
    { 199, 48.120f, 52.000f, 51.595f },
    { 200, 48.070f, 51.940f, 51.435f },
    { 201, 48.580f, 51.840f, 51.255f },
    { 202, 48.760f, 51.850f, 51.405f },
    { 203, 48.565f, 51.830f, 51.540f },
    { 204, 48.255f, 51.735f, 51.560f },
    { 205, 48.200f, 51.775f, 51.480f },
    { 206, 48.285f, 51.755f, 51.425f },
    { 207, 48.085f, 51.545f, 51.455f },
    { 208, 48.060f, 51.410f, 51.520f },
    { 209, 48.070f, 51.440f, 51.545f },
    { 210, 48.040f, 51.400f, 51.460f },
    { 211, 48.055f, 51.495f, 51.480f },
    { 212, 48.025f, 51.820f, 51.545f },
    { 213, 48.035f, 52.175f, 51.545f },
    { 214, 48.195f, 52.515f, 51.630f },
    { 215, 48.205f, 52.675f, 51.610f },
    { 216, 47.990f, 52.540f, 51.485f },
    { 217, 47.910f, 52.445f, 51.415f },
    { 218, 47.950f, 52.500f, 51.480f },
    { 219, 47.965f, 52.600f, 51.435f },
    { 220, 47.930f, 52.630f, 51.380f },
    { 221, 47.920f, 52.630f, 51.485f },
    { 222, 47.690f, 52.625f, 51.480f },
    { 223, 47.670f, 52.380f, 51.310f },
    { 224, 47.575f, 52.115f, 51.100f },
    { 225, 47.380f, 51.805f, 50.940f },
    { 226, 47.205f, 51.575f, 50.735f },
    { 227, 47.080f, 51.380f, 50.565f },
    { 228, 46.980f, 51.255f, 50.435f },
    { 229, 46.905f, 51.170f, 50.305f },
    { 230, 46.770f, 51.035f, 50.205f },
    { 231, 46.745f, 50.785f, 50.365f },
    { 232, 46.555f, 50.405f, 50.430f },
    { 233, 46.240f, 50.085f, 50.225f },
    { 234, 46.185f, 49.830f, 49.980f },
    { 235, 46.100f, 49.730f, 49.860f },
    { 236, 46.100f, 49.675f, 49.750f },
    { 237, 45.955f, 49.590f, 49.600f },
    { 238, 45.820f, 49.485f, 49.435f },
    { 239, 45.825f, 49.580f, 49.370f },
    { 240, 46.075f, 50.000f, 49.665f },
    { 241, 46.485f, 50.990f, 50.200f },
    { 242, 47.125f, 52.275f, 50.710f },
    { 243, 47.885f, 53.280f, 51.250f },
    { 244, 47.795f, 54.015f, 51.485f },
    { 245, 47.975f, 54.680f, 51.790f },
    { 246, 48.495f, 55.425f, 52.325f },
    { 247, 49.320f, 56.065f, 52.660f },
    { 248, 50.250f, 56.540f, 52.965f },
    { 249, 49.895f, 56.970f, 52.920f },
    { 250, 49.475f, 57.200f, 53.135f },
    { 251, 49.830f, 57.055f, 53.955f },
    { 252, 50.045f, 56.760f, 54.310f },
    { 253, 50.010f, 56.685f, 54.150f },
    { 254, 49.820f, 56.495f, 53.780f },
    { 255, 49.460f, 56.120f, 53.510f },
    { 256, 49.190f, 55.875f, 53.455f },
    { 257, 48.940f, 55.930f, 53.460f },
    { 258, 48.715f, 56.015f, 53.375f },
    { 259, 48.580f, 56.105f, 53.275f },
    { 260, 48.555f, 56.160f, 53.160f },
    { 261, 48.385f, 56.060f, 53.035f },
    { 262, 48.145f, 55.915f, 52.965f },
    { 263, 48.090f, 55.935f, 53.035f },
    { 264, 48.180f, 55.870f, 52.990f },
    { 265, 48.155f, 55.795f, 52.875f },
    { 266, 48.035f, 55.775f, 52.740f },
    { 267, 47.850f, 55.820f, 52.590f },
    { 268, 47.530f, 55.710f, 52.445f },
    { 269, 47.365f, 55.530f, 52.440f },
    { 270, 47.280f, 55.390f, 52.320f },
    { 271, 47.385f, 55.365f, 52.170f },
    { 272, 47.330f, 55.285f, 52.245f },
    { 273, 47.370f, 55.170f, 52.210f },
    { 274, 47.390f, 54.975f, 52.025f },
    { 275, 47.260f, 54.855f, 51.835f },
    { 276, 47.100f, 54.710f, 51.730f },
    { 277, 47.000f, 54.625f, 51.740f },
    { 278, 46.915f, 54.515f, 51.715f },
    { 279, 46.980f, 54.620f, 51.815f },
    { 280, 47.075f, 54.715f, 51.755f },
    { 281, 47.045f, 54.670f, 51.490f },
    { 282, 47.135f, 54.755f, 51.290f },
    { 283, 47.270f, 54.950f, 51.250f },
};

// Index of the NEXT sample to send.
static size_t current_sample_index = 0;

// ================= OLED HELPERS =================

static void show_ready_sample(void)
{
    static char display_text[128];

    if (current_sample_index >= HIVE_WEIGHT_SAMPLE_COUNT) {
        oled_show_message(
            "HIVE DATA",
            "ALL DATA SENT\n"
            "DAY 120-283\n"
            "COMPLETED"
        );
        return;
    }

    const hive_weight_sample_t *sample =
        &hive_weight_data[current_sample_index];

    snprintf(
        display_text,
        sizeof(display_text),
        "DAY %u READY\n"
        "H1: %.3f\n"
        "H2: %.3f\n"
        "H3: %.3f\n"
        "CW: SEND",
        sample->day,
        sample->hive1,
        sample->hive2,
        sample->hive3
    );

    oled_show_message("HIVE DATA", display_text);
}

static void show_sent_next(uint16_t sent_day)
{
    static char display_text[128];

    if (current_sample_index >= HIVE_WEIGHT_SAMPLE_COUNT) {
        snprintf(
            display_text,
            sizeof(display_text),
            "DAY %u SENT\n"
            "ALL DATA SENT\n"
            "DAY 120-283\n"
            "COMPLETED",
            sent_day
        );
    } else {
        snprintf(
            display_text,
            sizeof(display_text),
            "DAY %u SENT\n"
            "NEXT: DAY %u\n"
            "CW: SEND NEXT",
            sent_day,
            hive_weight_data[current_sample_index].day
        );
    }

    oled_show_message("HIVE DATA", display_text);
}

// ================= SEND ONE SAMPLE =================

static bool send_current_hive_sample(void)
{
    static char sms_message[200];
    static char display_text[96];

    if (current_sample_index >= HIVE_WEIGHT_SAMPLE_COUNT) {
        show_ready_sample();
        return false;
    }

    const hive_weight_sample_t *sample =
        &hive_weight_data[current_sample_index];

    int csq = 99;
    int rssi_dbm = 0;

    snprintf(
        display_text,
        sizeof(display_text),
        "DAY %u\n"
        "CHECKING GSM...\n"
        "PLEASE WAIT",
        sample->day
    );
    oled_show_message("HIVE DATA", display_text);

    bool registered = modem_is_registered();
    bool signal_ok = modem_get_signal_strength(&csq, &rssi_dbm);

    if (!registered) {
        snprintf(
            display_text,
            sizeof(display_text),
            "DAY %u NOT SENT\n"
            "NO GSM NETWORK\n"
            "CW: RETRY",
            sample->day
        );
        oled_show_message("HIVE DATA", display_text);
        ESP_LOGW(TAG, "Day %u not sent: GSM not registered", sample->day);
        return false;
    }

    // Read the current local date/time directly from the GSM network
    // immediately before preparing the SMS.
    struct tm gsm_time = {0};

    oled_show_message(
        "HIVE DATA",
        "READING GSM\n"
        "DATE & TIME..."
    );

    if (!modem_get_network_time(&gsm_time)) {
        snprintf(
            display_text,
            sizeof(display_text),
            "DAY %u NOT SENT\n"
            "TIME ERROR\n"
            "CW: RETRY",
            sample->day
        );
        oled_show_message("HIVE DATA", display_text);

        ESP_LOGE(
            TAG,
            "Day %u not sent: unable to read GSM network time",
            sample->day
        );

        return false;
    }

    // Required SMS format:
    //
    // Hive Weight Data
    // 2026-08-19 14:32:00
    // Hive 1: 42.80
    // Hive 2: 38.60
    // Hive 3: 47.20
    snprintf(
        sms_message,
        sizeof(sms_message),
        "Hive Weight Data\n"
        "%04d-%02d-%02d %02d:%02d:%02d\n"
        "Hive 1: %.2f\n"
        "Hive 2: %.2f\n"
        "Hive 3: %.2f",
        gsm_time.tm_year + 1900,
        gsm_time.tm_mon + 1,
        gsm_time.tm_mday,
        gsm_time.tm_hour,
        gsm_time.tm_min,
        gsm_time.tm_sec,
        sample->hive1,
        sample->hive2,
        sample->hive3
    );

    snprintf(
        display_text,
        sizeof(display_text),
        "DAY %u\n"
        "SENDING SMS...\n"
        "PLEASE WAIT",
        sample->day
    );
    oled_show_message("HIVE DATA", display_text);

    ESP_LOGI(
        TAG,
        "Sending day %u at %04d-%02d-%02d %02d:%02d:%02d: "
        "H1=%.2f H2=%.2f H3=%.2f CSQ=%d RSSI=%d",
        sample->day,
        gsm_time.tm_year + 1900,
        gsm_time.tm_mon + 1,
        gsm_time.tm_mday,
        gsm_time.tm_hour,
        gsm_time.tm_min,
        gsm_time.tm_sec,
        sample->hive1,
        sample->hive2,
        sample->hive3,
        signal_ok ? csq : 99,
        signal_ok ? rssi_dbm : 0
    );

    bool sms_sent = modem_send_sms(sms_message);

    if (!sms_sent) {
        snprintf(
            display_text,
            sizeof(display_text),
            "DAY %u FAILED\n"
            "SAME DAY KEPT\n"
            "CW: RETRY",
            sample->day
        );
        oled_show_message("HIVE DATA", display_text);
        ESP_LOGE(TAG, "SMS failed for day %u; index not advanced", sample->day);
        return false;
    }

    uint16_t sent_day = sample->day;

    // Advance to the next dataset row ONLY after a successful SMS.
    current_sample_index++;

    ESP_LOGI(TAG, "Day %u sent successfully", sent_day);
    show_sent_next(sent_day);

    return true;
}


// ================= MAIN =================

void app_main(void)
{
    bool modem_ready = false;
    int64_t last_sms_time = 0;

    ESP_LOGI(TAG, "Starting sequential hive SMS sender");

    // 1. OLED
    oled_init();
    oled_show_message(
        "HIVE DATA",
        "STARTING..."
    );

    // 2. Rotary encoder
    encoder_init();

    vTaskDelay(pdMS_TO_TICKS(500));

    // 3. GSM modem
    oled_show_message(
        "HIVE DATA",
        "INITIALIZING\n"
        "GSM MODEM..."
    );

    modem_ready = modem_init();

    while (!modem_ready) {
        ESP_LOGE(TAG, "Modem initialization failed");

        oled_show_message(
            "MODEM ERROR",
            "NO RESPONSE\n"
            "RETRYING..."
        );

        vTaskDelay(pdMS_TO_TICKS(5000));
        modem_ready = modem_init();
    }

    // 4. Wait for network registration
    oled_show_message(
        "HIVE DATA",
        "MODEM READY\n"
        "WAITING GSM..."
    );

    while (!modem_is_registered()) {
        ESP_LOGW(TAG, "Waiting for GSM network registration");

        oled_show_message(
            "HIVE DATA",
            "WAITING FOR\n"
            "GSM NETWORK..."
        );

        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    ESP_LOGI(TAG, "GSM registered. Ready at day 120.");

    // 5. First record ready: Day 120
    show_ready_sample();

    // 6. Sequential send loop
    while (true) {
        encoder_direction_t direction = encoder_get_direction();

        if (direction == ENCODER_CLOCKWISE) {
            int64_t now_us = esp_timer_get_time();

            if (current_sample_index >= HIVE_WEIGHT_SAMPLE_COUNT) {
                show_ready_sample();
            }
            else if (
                now_us - last_sms_time >=
                ((int64_t)SMS_COOLDOWN_MS * 1000)
            ) {
                // Start cooldown immediately so one CW event cannot
                // accidentally trigger multiple SMS transmissions.
                last_sms_time = now_us;

                bool sent = send_current_hive_sample();

                if (sent) {
                    // Keep the "DAY xxx SENT / NEXT DAY xxx" status visible.
                    vTaskDelay(pdMS_TO_TICKS(SMS_STATUS_DELAY_MS));
                    show_ready_sample();
                }
            }
        }
        else if (direction == ENCODER_COUNTERCLOCKWISE) {
            // Counterclockwise does NOT change the sequence.
            // It only refreshes the current day on the OLED.
            ESP_LOGI(TAG, "CCW detected - sequence unchanged");
            show_ready_sample();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}