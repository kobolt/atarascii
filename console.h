#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>
#include "tia.h"

void console_pause(void);
void console_resume(void);
void console_exit(void);
void console_init(bool vblank_strip, bool disable_colors);
uint8_t console_get_system_switches(void);
uint8_t console_get_joystick_movement(void);
bool console_get_joystick_button_p0(void);
bool console_get_joystick_button_p1(void);
void console_draw_scanline(uint16_t y, uint8_t colors[], tia_object_t object[]);
void console_update(void);

#endif /* _CONSOLE_H */
