#include "oled.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

// =====================================================
// 1.3-INCH SH1106 OLED CONFIGURATION
// =====================================================

#define OLED_I2C_PORT          I2C_NUM_0
#define OLED_SDA_PIN           GPIO_NUM_8
#define OLED_SCL_PIN           GPIO_NUM_9
#define OLED_I2C_ADDRESS       0x3C
#define OLED_I2C_SPEED_HZ      400000

#define OLED_WIDTH             128
#define OLED_HEIGHT            64
#define OLED_PAGES             8
#define OLED_COLUMN_OFFSET     2
#define OLED_BUFFER_SIZE       (OLED_WIDTH * OLED_PAGES)

#define OLED_COMMAND_CONTROL   0x00
#define OLED_DATA_CONTROL      0x40

static const char *TAG = "OLED";

static uint8_t oled_buffer[OLED_BUFFER_SIZE];
static bool oled_initialized = false;

// =====================================================
// 5x7 FONT
// =====================================================

static const uint8_t font_5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},
    {0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},
    {0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},

    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},

    {0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E},

    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},

    {0x00,0x7F,0x41,0x41,0x00},{0x02,0x04,0x08,0x10,0x20},
    {0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},

    {0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},
    {0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},
    {0x0C,0x52,0x52,0x52,0x3E},{0x7F,0x08,0x04,0x04,0x78},
    {0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
    {0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},
    {0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},
    {0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},
    {0x48,0x54,0x54,0x54,0x20},{0x04,0x3F,0x44,0x40,0x20},
    {0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},
    {0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},

    {0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x7F,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00},{0x08,0x04,0x08,0x10,0x08},
    {0x00,0x00,0x00,0x00,0x00}
};

// =====================================================
// LOW-LEVEL I2C
// =====================================================

static esp_err_t oled_write_command(
    uint8_t command
)
{
    uint8_t data[2] = {
        OLED_COMMAND_CONTROL,
        command
    };

    return i2c_master_write_to_device(
        OLED_I2C_PORT,
        OLED_I2C_ADDRESS,
        data,
        sizeof(data),
        pdMS_TO_TICKS(100)
    );
}

static esp_err_t oled_write_data(
    const uint8_t *data,
    size_t length
)
{
    if (
        data == NULL ||
        length == 0 ||
        length > OLED_WIDTH
    )
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t tx_buffer[
        OLED_WIDTH + 1
    ];

    tx_buffer[0] =
        OLED_DATA_CONTROL;

    memcpy(
        &tx_buffer[1],
        data,
        length
    );

    return i2c_master_write_to_device(
        OLED_I2C_PORT,
        OLED_I2C_ADDRESS,
        tx_buffer,
        length + 1,
        pdMS_TO_TICKS(100)
    );
}

static esp_err_t oled_i2c_init(void)
{
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_SDA_PIN,
        .scl_io_num = OLED_SCL_PIN,
        .sda_pullup_en =
            GPIO_PULLUP_ENABLE,
        .scl_pullup_en =
            GPIO_PULLUP_ENABLE,
        .master.clk_speed =
            OLED_I2C_SPEED_HZ,
        .clk_flags = 0
    };

    esp_err_t result =
        i2c_param_config(
            OLED_I2C_PORT,
            &config
        );

    if (result != ESP_OK)
    {
        return result;
    }

    result =
        i2c_driver_install(
            OLED_I2C_PORT,
            I2C_MODE_MASTER,
            0,
            0,
            0
        );

    if (
        result ==
        ESP_ERR_INVALID_STATE
    )
    {
        return ESP_OK;
    }

    return result;
}

// =====================================================
// DRAWING
// =====================================================

static void oled_set_pixel(
    int x,
    int y,
    bool enabled
)
{
    if (
        x < 0 ||
        x >= OLED_WIDTH ||
        y < 0 ||
        y >= OLED_HEIGHT
    )
    {
        return;
    }

    uint16_t index =
        ((uint16_t)y / 8U) *
        OLED_WIDTH +
        (uint16_t)x;

    uint8_t mask =
        (uint8_t)(
            1U <<
            ((uint8_t)y % 8U)
        );

    if (enabled)
    {
        oled_buffer[index] |= mask;
    }
    else
    {
        oled_buffer[index] &=
            (uint8_t)(~mask);
    }
}

static void oled_draw_character(
    int x,
    int y,
    char character
)
{
    if (
        character < 32 ||
        character > 127
    )
    {
        character = '?';
    }

    const uint8_t *glyph =
        font_5x7[
            (uint8_t)character - 32U
        ];

    for (
        int column = 0;
        column < 5;
        column++
    )
    {
        uint8_t column_data =
            glyph[column];

        for (
            int row = 0;
            row < 7;
            row++
        )
        {
            oled_set_pixel(
                x + column,
                y + row,
                (
                    column_data &
                    (1U << row)
                ) != 0
            );
        }
    }
}

static void oled_draw_text(
    int x,
    int y,
    const char *text
)
{
    if (text == NULL)
    {
        return;
    }

    int cursor_x = x;
    int cursor_y = y;

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            cursor_x = x;
            cursor_y += 10;
            text++;
            continue;
        }

        if (
            cursor_x + 6 >
            OLED_WIDTH
        )
        {
            cursor_x = x;
            cursor_y += 10;
        }

        if (
            cursor_y + 8 >
            OLED_HEIGHT
        )
        {
            break;
        }

        oled_draw_character(
            cursor_x,
            cursor_y,
            *text
        );

        cursor_x += 6;
        text++;
    }
}

static esp_err_t oled_refresh(void)
{
    for (
        uint8_t page = 0;
        page < OLED_PAGES;
        page++
    )
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            oled_write_command(
                (uint8_t)(
                    0xB0U + page
                )
            )
        );

        ESP_ERROR_CHECK_WITHOUT_ABORT(
            oled_write_command(
                (uint8_t)(
                    0x00U +
                    (
                        OLED_COLUMN_OFFSET &
                        0x0FU
                    )
                )
            )
        );

        ESP_ERROR_CHECK_WITHOUT_ABORT(
            oled_write_command(
                (uint8_t)(
                    0x10U +
                    (
                        (
                            OLED_COLUMN_OFFSET >>
                            4U
                        ) &
                        0x0FU
                    )
                )
            )
        );

        esp_err_t result =
            oled_write_data(
                &oled_buffer[
                    page *
                    OLED_WIDTH
                ],
                OLED_WIDTH
            );

        if (result != ESP_OK)
        {
            return result;
        }
    }

    return ESP_OK;
}

// =====================================================
// PUBLIC FUNCTIONS
// =====================================================

void oled_clear(void)
{
    memset(
        oled_buffer,
        0,
        sizeof(oled_buffer)
    );

    if (oled_initialized)
    {
        oled_refresh();
    }
}

void oled_init(void)
{
    ESP_LOGI(
        TAG,
        "Initializing SH1106 OLED"
    );

    esp_err_t result =
        oled_i2c_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "I2C initialization failed: %s",
            esp_err_to_name(result)
        );

        return;
    }

    const uint8_t commands[] = {
        0xAE,
        0xD5, 0x80,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0xAD, 0x8B,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0x7F,
        0xD9, 0x22,
        0xDB, 0x35,
        0xA4,
        0xA6,
        0xAF
    };

    for (
        size_t i = 0;
        i < sizeof(commands);
        i++
    )
    {
        result =
            oled_write_command(
                commands[i]
            );

        if (result != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "OLED command failed"
            );

            return;
        }
    }

    oled_initialized = true;

    oled_show_message(
        "LORA GATEWAY",
        "INITIALIZING..."
    );

    ESP_LOGI(
        TAG,
        "OLED initialized successfully"
    );
}

void oled_show_message(
    const char *title,
    const char *message
)
{
    if (!oled_initialized)
    {
        return;
    }

    memset(
        oled_buffer,
        0,
        sizeof(oled_buffer)
    );

    oled_draw_text(
        0,
        0,
        title != NULL
            ? title
            : ""
    );

    oled_draw_text(
        0,
        14,
        message != NULL
            ? message
            : ""
    );

    oled_refresh();
}

void oled_show_range_data(
    double distance_m,
    int rssi_dbm,
    float snr_db,
    double packet_loss_percent,
    uint32_t received_packets,
    uint32_t expected_packets,
    bool reliable
)
{
    if (!oled_initialized)
    {
        return;
    }

    char line1[24];
    char line2[24];
    char line3[24];
    char line4[24];
    char line5[24];

    snprintf(
        line1,
        sizeof(line1),
        "DIST:%.1fm",
        distance_m
    );

    snprintf(
        line2,
        sizeof(line2),
        "RSSI:%ddBm",
        rssi_dbm
    );

    snprintf(
        line3,
        sizeof(line3),
        "SNR:%.1fdB",
        snr_db
    );

    snprintf(
        line4,
        sizeof(line4),
        "PKT:%lu/%lu",
        (unsigned long)
        received_packets,
        (unsigned long)
        expected_packets
    );

    snprintf(
        line5,
        sizeof(line5),
        "LOSS:%.1f%% %s",
        packet_loss_percent,
        reliable ? "OK" : "NO"
    );

    memset(
        oled_buffer,
        0,
        sizeof(oled_buffer)
    );

    oled_draw_text(
        0,
        0,
        line1
    );

    oled_draw_text(
        0,
        11,
        line2
    );

    oled_draw_text(
        0,
        22,
        line3
    );

    oled_draw_text(
        0,
        33,
        line4
    );

    oled_draw_text(
        0,
        44,
        line5
    );

    oled_refresh();
}
