#ifndef OLED_H
#define OLED_H

/**
 * @brief Initialize the 1.3-inch SH1106 OLED.
 *
 * Connections:
 * SDA     -> GPIO8
 * SCL     -> GPIO9
 * Address -> 0x3C
 */
void oled_init(void);

/**
 * @brief Clear the complete OLED display.
 */
void oled_clear(void);

/**
 * @brief Display a title and multiline message.
 *
 * Use '\n' to move to the next line.
 *
 * Example:
 *
 * oled_show_message(
 *     "GSM SIGNAL TEST",
 *     "CSQ: 18/31\n"
 *     "RSSI: -77 dBm\n"
 *     "Press: SEND SMS"
 * );
 *
 * @param title Top-line title text.
 * @param message Multiline display text.
 */
void oled_show_message(
    const char *title,
    const char *message
);

#endif