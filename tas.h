#ifndef _TAS_H
#define _TAS_H

int tas_init(const char *filename);
uint8_t tas_get_system_switches(void);
uint8_t tas_get_joystick_movement(void);
bool tas_get_joystick_button_p0(void);
bool tas_get_joystick_button_p1(void);
void tas_update(void);
bool tas_is_active(void);

#endif /* _TAS_H */
