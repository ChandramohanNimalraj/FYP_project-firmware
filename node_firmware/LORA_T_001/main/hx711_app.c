#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_rom_sys.h"

#include "hx711_app.h"

// ================= HX711 PINS =================
#define HX711_DT   GPIO_NUM_8
#define HX711_SCK  GPIO_NUM_9

// ================= CALIBRATION =================

// Tare offset
static long offset = 0;

// Scale factor
static float scale = 1.0;

// ================= HX711 READ =================
static long hx711_read()
{
    long count = 0;

    // Wait until HX711 ready
    while (gpio_get_level(HX711_DT));

    // Read 24 bits
    for (int i = 0; i < 24; i++)
    {
        gpio_set_level(HX711_SCK, 1);

        esp_rom_delay_us(1);

        count = count << 1;

        gpio_set_level(HX711_SCK, 0);

        esp_rom_delay_us(1);

        if (gpio_get_level(HX711_DT))
        {
            count++;
        }
    }

    // Set gain
    gpio_set_level(HX711_SCK, 1);

    esp_rom_delay_us(1);

    gpio_set_level(HX711_SCK, 0);

    esp_rom_delay_us(1);

    // Convert signed value
    if (count & 0x800000)
    {
        count |= ~0xFFFFFF;
    }

    return count;
}

// ================= AVERAGE READ =================
static long hx711_read_average(int times)
{
    long sum = 0;

    for (int i = 0; i < times; i++)
    {
        sum += hx711_read();
    }

    return sum / times;
}

// ================= TARE =================
static void hx711_tare()
{
    printf("\nRemove all weight...\n");

    vTaskDelay(pdMS_TO_TICKS(5000));

    offset = hx711_read_average(20);

    printf("Offset = %ld\n", offset);
}

// ================= INIT =================
void hx711_app_init(void)
{
    gpio_config_t io_conf = {};

    // DT INPUT
    io_conf.pin_bit_mask =
        (1ULL << HX711_DT);

    io_conf.mode =
        GPIO_MODE_INPUT;

    io_conf.pull_up_en =
        GPIO_PULLUP_DISABLE;

    io_conf.pull_down_en =
        GPIO_PULLDOWN_DISABLE;

    io_conf.intr_type =
        GPIO_INTR_DISABLE;

    gpio_config(&io_conf);

    // SCK OUTPUT
    io_conf.pin_bit_mask =
        (1ULL << HX711_SCK);

    io_conf.mode =
        GPIO_MODE_OUTPUT;

    gpio_config(&io_conf);

    gpio_set_level(HX711_SCK, 0);

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("\n");
    printf("=============================\n");
    printf("     HX711 INITIALIZED\n");
    printf("=============================\n");

    // ================= TARE =================
    hx711_tare();

    // ================= CALIBRATION =================

    printf("\nPlace known weight now...\n");

    printf("Waiting 10 seconds...\n");

    vTaskDelay(pdMS_TO_TICKS(10000));

    long raw =
        hx711_read_average(20);

    printf("Raw Reading = %ld\n",
           raw);

    // CHANGE THIS TO YOUR KNOWN WEIGHT
    float known_weight = 220.0;

    scale =
        (raw - offset)
        /
        known_weight;

    printf("\nCalibration Complete\n");

    printf("Scale Factor = %.2f\n",
           scale);

    printf("=============================\n");
}

// ================= GET WEIGHT =================
float hx711_get_weight(void)
{
    long reading =
        hx711_read_average(5);

    float weight =
        (reading - offset)
        /
        scale;

    printf("Weight = %.2f g\n",
           weight);

    return weight;
}