#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

void oled_init(void);

void oled_clear(void);

void oled_show_message(
    const char *title,
    const char *message
);

#ifdef __cplusplus
}
#endif

#endif