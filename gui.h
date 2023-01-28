#ifndef _GUI_H
#define _GUI_H

#include <stdint.h>
#include <stdbool.h>

int gui_init(int joystick_no, bool disable_video, bool disable_audio);
void gui_draw_scanline(uint16_t y, uint8_t colors[]);
uint8_t gui_get_system_switches(void);
uint8_t gui_get_joystick_movement(void);
bool gui_get_joystick_button_p0(void);
bool gui_get_joystick_button_p1(void);
bool gui_save_state_requested(void);
bool gui_load_state_requested(void);
void gui_update(void);

#endif /* _GUI_H */
