#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "oled.h"

// ================= OLED CONFIGURATION =================

#define OLED_I2C_PORT          I2C_NUM_0

#define OLED_SDA_PIN           8
#define OLED_SCL_PIN           9

#define OLED_I2C_ADDRESS       0x3C
#define OLED_I2C_SPEED_HZ      400000

#define OLED_WIDTH             128
#define OLED_HEIGHT            64
#define OLED_PAGES             (OLED_HEIGHT / 8)

#define OLED_COLUMN_OFFSET     2
#define OLED_BUFFER_SIZE       (OLED_WIDTH * OLED_PAGES)

#define OLED_COMMAND_CONTROL   0x00
#define OLED_DATA_CONTROL      0x40

static const char *TAG = "SH1106";

static uint8_t oled_buffer[OLED_BUFFER_SIZE];
static bool oled_initialized = false;

// ================= 5x7 FONT =================

static const uint8_t font_5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // Space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /

    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9

    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x00,0x41,0x22,0x14,0x08}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @

    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z

    {0x00,0x7F,0x41,0x41,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20}, // Backslash
    {0x00,0x41,0x41,0x7F,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04}, // ^
    {0x40,0x40,0x40,0x40,0x40}, // _
    {0x00,0x01,0x02,0x04,0x00}, // `

    {0x20,0x54,0x54,0x54,0x78}, // a
    {0x7F,0x48,0x44,0x44,0x38}, // b
    {0x38,0x44,0x44,0x44,0x20}, // c
    {0x38,0x44,0x44,0x48,0x7F}, // d
    {0x38,0x54,0x54,0x54,0x18}, // e
    {0x08,0x7E,0x09,0x01,0x02}, // f
    {0x0C,0x52,0x52,0x52,0x3E}, // g
    {0x7F,0x08,0x04,0x04,0x78}, // h
    {0x00,0x44,0x7D,0x40,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00}, // j
    {0x7F,0x10,0x28,0x44,0x00}, // k
    {0x00,0x41,0x7F,0x40,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78}, // m
    {0x7C,0x08,0x04,0x04,0x78}, // n
    {0x38,0x44,0x44,0x44,0x38}, // o
    {0x7C,0x14,0x14,0x14,0x08}, // p
    {0x08,0x14,0x14,0x18,0x7C}, // q
    {0x7C,0x08,0x04,0x04,0x08}, // r
    {0x48,0x54,0x54,0x54,0x20}, // s
    {0x04,0x3F,0x44,0x40,0x20}, // t
    {0x3C,0x40,0x40,0x20,0x7C}, // u
    {0x1C,0x20,0x40,0x20,0x1C}, // v
    {0x3C,0x40,0x30,0x40,0x3C}, // w
    {0x44,0x28,0x10,0x28,0x44}, // x
    {0x0C,0x50,0x50,0x50,0x3C}, // y
    {0x44,0x64,0x54,0x4C,0x44}, // z

    {0x00,0x08,0x36,0x41,0x00}, // {
    {0x00,0x00,0x7F,0x00,0x00}, // |
    {0x00,0x41,0x36,0x08,0x00}, // }
    {0x08,0x04,0x08,0x10,0x08}, // ~
    {0x00,0x00,0x00,0x00,0x00}
};

// ================= I2C FUNCTIONS =================

static esp_err_t oled_write_command(uint8_t command)
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
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t transmission_buffer[OLED_WIDTH + 1];

    if (length > OLED_WIDTH) {
        return ESP_ERR_INVALID_SIZE;
    }

    transmission_buffer[0] = OLED_DATA_CONTROL;

    memcpy(
        &transmission_buffer[1],
        data,
        length
    );

    return i2c_master_write_to_device(
        OLED_I2C_PORT,
        OLED_I2C_ADDRESS,
        transmission_buffer,
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
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = OLED_I2C_SPEED_HZ,
        .clk_flags = 0
    };

    esp_err_t result = i2c_param_config(
        OLED_I2C_PORT,
        &config
    );

    if (result != ESP_OK) {
        return result;
    }

    result = i2c_driver_install(
        OLED_I2C_PORT,
        I2C_MODE_MASTER,
        0,
        0,
        0
    );

    if (result == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(
            TAG,
            "I2C driver already installed"
        );

        return ESP_OK;
    }

    return result;
}

// ================= INITIALIZATION SEQUENCE =================

static esp_err_t oled_send_initialization_sequence(void)
{
    const uint8_t initialization_commands[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Display clock
        0xA8, 0x3F, // Multiplex ratio: 64
        0xD3, 0x00, // Display offset
        0x40,       // Display start line
        0xAD, 0x8B, // DC-DC converter ON
        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pin configuration
        0x81, 0x7F, // Contrast
        0xD9, 0x22, // Pre-charge period
        0xDB, 0x35, // VCOM deselect level
        0xA4,       // Display follows RAM
        0xA6,       // Normal display
        0xAF        // Display ON
    };

    for (
        size_t index = 0;
        index < sizeof(initialization_commands);
        index++
    ) {
        esp_err_t result = oled_write_command(
            initialization_commands[index]
        );

        if (result != ESP_OK) {
            return result;
        }
    }

    return ESP_OK;
}

// ================= GRAPHICS FUNCTIONS =================

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
    ) {
        return;
    }

    uint16_t index =
        ((uint16_t)y / 8U) * OLED_WIDTH +
        (uint16_t)x;

    uint8_t mask = (uint8_t)(
        1U << ((uint8_t)y % 8U)
    );

    if (enabled) {
        oled_buffer[index] |= mask;
    } else {
        oled_buffer[index] &= (uint8_t)(~mask);
    }
}

static void oled_draw_character(
    int x,
    int y,
    char character,
    uint8_t scale
)
{
    if (scale == 0) {
        scale = 1;
    }

    if (
        character < 32 ||
        character > 127
    ) {
        character = '?';
    }

    const uint8_t *glyph =
        font_5x7[(uint8_t)character - 32U];

    for (int column = 0; column < 5; column++) {
        uint8_t column_data = glyph[column];

        for (int row = 0; row < 7; row++) {
            bool pixel_enabled =
                (column_data & (1U << row)) != 0;

            for (
                uint8_t scale_x = 0;
                scale_x < scale;
                scale_x++
            ) {
                for (
                    uint8_t scale_y = 0;
                    scale_y < scale;
                    scale_y++
                ) {
                    oled_set_pixel(
                        x + column * scale + scale_x,
                        y + row * scale + scale_y,
                        pixel_enabled
                    );
                }
            }
        }
    }
}

static void oled_draw_text(
    int x,
    int y,
    const char *text,
    uint8_t scale
)
{
    if (text == NULL) {
        return;
    }

    if (scale == 0) {
        scale = 1;
    }

    int cursor_x = x;
    int cursor_y = y;

    int character_width = 6 * scale;
    int character_height = 8 * scale;

    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = x;
            cursor_y += character_height;
            text++;
            continue;
        }

        if (
            cursor_x + character_width >
            OLED_WIDTH
        ) {
            cursor_x = x;
            cursor_y += character_height;
        }

        if (
            cursor_y + character_height >
            OLED_HEIGHT
        ) {
            break;
        }

        oled_draw_character(
            cursor_x,
            cursor_y,
            *text,
            scale
        );

        cursor_x += character_width;
        text++;
    }
}

static esp_err_t oled_refresh(void)
{
    for (
        uint8_t page = 0;
        page < OLED_PAGES;
        page++
    ) {
        esp_err_t result;

        result = oled_write_command(
            (uint8_t)(0xB0U + page)
        );

        if (result != ESP_OK) {
            return result;
        }

        result = oled_write_command(
            (uint8_t)(
                0x00U +
                (OLED_COLUMN_OFFSET & 0x0FU)
            )
        );

        if (result != ESP_OK) {
            return result;
        }

        result = oled_write_command(
            (uint8_t)(
                0x10U +
                ((OLED_COLUMN_OFFSET >> 4U) & 0x0FU)
            )
        );

        if (result != ESP_OK) {
            return result;
        }

        result = oled_write_data(
            &oled_buffer[
                page * OLED_WIDTH
            ],
            OLED_WIDTH
        );

        if (result != ESP_OK) {
            return result;
        }
    }

    return ESP_OK;
}

// ================= PUBLIC FUNCTIONS =================

void oled_clear(void)
{
    memset(
        oled_buffer,
        0x00,
        sizeof(oled_buffer)
    );

    if (oled_initialized) {
        esp_err_t result = oled_refresh();

        if (result != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Clear failed: %s",
                esp_err_to_name(result)
            );
        }
    }
}

void oled_init(void)
{
    ESP_LOGI(
        TAG,
        "Initializing 1.3-inch SH1106 OLED"
    );

    esp_err_t result = oled_i2c_init();

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C initialization failed: %s",
            esp_err_to_name(result)
        );

        return;
    }

    result = oled_send_initialization_sequence();

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OLED initialization failed: %s",
            esp_err_to_name(result)
        );

        return;
    }

    oled_initialized = true;

    memset(
        oled_buffer,
        0x00,
        sizeof(oled_buffer)
    );

    oled_draw_text(
        22,
        16,
        "OLED",
        2
    );

    oled_draw_text(
        47,
        40,
        "Ready",
        1
    );

    result = oled_refresh();

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Initial display failed: %s",
            esp_err_to_name(result)
        );

        oled_initialized = false;
        return;
    }

    ESP_LOGI(
        TAG,
        "SH1106 OLED initialized successfully"
    );
}

void oled_show_message(
    const char *title,
    const char *message
)
{
    if (!oled_initialized) {
        ESP_LOGE(
            TAG,
            "OLED is not initialized"
        );

        return;
    }

    memset(
        oled_buffer,
        0x00,
        sizeof(oled_buffer)
    );

    oled_draw_text(
        0,
        0,
        title != NULL ? title : "",
        1
    );

    oled_draw_text(
        0,
        20,
        message != NULL ? message : "",
        1
    );

    esp_err_t result = oled_refresh();

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OLED refresh failed: %s",
            esp_err_to_name(result)
        );
    }
}