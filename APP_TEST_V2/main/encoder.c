#include "encoder.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ================= PIN CONFIGURATION =================

#define ENCODER_PIN_A       GPIO_NUM_19
#define ENCODER_PIN_B       GPIO_NUM_20
#define ENCODER_SWITCH_PIN  GPIO_NUM_21

#define BUTTON_DEBOUNCE_MS  50

static const char *TAG = "ENCODER";

// ================= INTERNAL VARIABLES =================

static int previous_a_state = 1;

static int last_raw_button_state = 1;
static int stable_button_state = 1;

static TickType_t last_button_change_time = 0;

// ================= INITIALIZATION =================

void encoder_init(void)
{
    gpio_config_t encoder_config = {
        .pin_bit_mask =
            (1ULL << ENCODER_PIN_A) |
            (1ULL << ENCODER_PIN_B) |
            (1ULL << ENCODER_SWITCH_PIN),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&encoder_config);

    previous_a_state =
        gpio_get_level(ENCODER_PIN_A);

    last_raw_button_state =
        gpio_get_level(ENCODER_SWITCH_PIN);

    stable_button_state =
        last_raw_button_state;

    last_button_change_time =
        xTaskGetTickCount();

    ESP_LOGI(
        TAG,
        "Encoder initialized. Button initial state: %d",
        stable_button_state
    );
}

// ================= ROTATION DETECTION =================

encoder_direction_t encoder_get_direction(void)
{
    int current_a_state =
        gpio_get_level(ENCODER_PIN_A);

    encoder_direction_t direction =
        ENCODER_NO_MOVEMENT;

    if (
        previous_a_state == 1 &&
        current_a_state == 0
    ) {
        int current_b_state =
            gpio_get_level(ENCODER_PIN_B);

        if (current_b_state == 1) {
            direction = ENCODER_CLOCKWISE;
        } else {
            direction = ENCODER_COUNTERCLOCKWISE;
        }
    }

    previous_a_state =
        current_a_state;

    return direction;
}

// ================= BUTTON DETECTION =================

bool encoder_button_pressed(void)
{
    int raw_button_state =
        gpio_get_level(ENCODER_SWITCH_PIN);

    TickType_t current_time =
        xTaskGetTickCount();

    /*
     * Restart debounce timer whenever the raw input changes.
     */
    if (raw_button_state != last_raw_button_state) {
        last_button_change_time =
            current_time;

        last_raw_button_state =
            raw_button_state;
    }

    /*
     * Accept the new state only when it remains stable
     * for the full debounce period.
     */
    if (
        (current_time - last_button_change_time) >=
        pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)
    ) {
        if (raw_button_state != stable_button_state) {
            stable_button_state =
                raw_button_state;

            /*
             * Internal pull-up:
             * HIGH = released
             * LOW  = pressed
             */
            if (stable_button_state == 0) {
                ESP_LOGI(TAG, "Encoder button pressed");
                return true;
            }
        }
    }

    return false;
}