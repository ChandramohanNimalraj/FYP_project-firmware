#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>

typedef enum
{
    ENCODER_NO_MOVEMENT = 0,
    ENCODER_CLOCKWISE,
    ENCODER_COUNTERCLOCKWISE
} encoder_direction_t;

/**
 * @brief Initialize the rotary encoder.
 *
 * Connections:
 * Encoder A      -> GPIO19
 * Encoder B      -> GPIO20
 * Encoder switch -> GPIO21
 */
void encoder_init(void);

/**
 * @brief Check the rotary encoder direction.
 *
 * @return ENCODER_CLOCKWISE,
 *         ENCODER_COUNTERCLOCKWISE,
 *         or ENCODER_NO_MOVEMENT.
 */
encoder_direction_t encoder_get_direction(void);

/**
 * @brief Check whether the encoder push button was pressed.
 *
 * The function includes software debounce and returns true
 * only once for each press.
 *
 * @return true when a new button press is detected.
 */
bool encoder_button_pressed(void);

#endif