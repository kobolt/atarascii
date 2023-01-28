#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>



#define TAS_DATA_MAX 81920



static uint8_t tas_system_switches[TAS_DATA_MAX];
static uint8_t tas_joystick_movement[TAS_DATA_MAX];
static bool tas_joystick_button_p0[TAS_DATA_MAX];
static bool tas_joystick_button_p1[TAS_DATA_MAX];
static unsigned int tas_data_index = 0;
static unsigned int tas_data_end = 0;
static bool tas_active = false;



int tas_init(const char *filename)
{
  int n, result;
  FILE *fh;
  char buffer[32];
  uint8_t system_switches;
  uint8_t joystick_movement;
  bool joystick_button_p0;
  bool joystick_button_p1;
  char p0u, p0d, p0l, p0r, p0b, p1u, p1d, p1l, p1r, p1b, s, r;

  fh = fopen(filename, "r");
  if (fh == NULL) {
    return -1;
  }

  tas_data_index = 0;
  tas_active = true;

  n = 0;
  while (fgets(buffer, sizeof(buffer), fh) != NULL) {
    /* Format: p0u,p0d,p0l,p0r,p0b,p1u,p1d,p1l,p1r,p1b,s,r */
    result = sscanf(buffer, "%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c\n",
      &p0u, &p0d, &p0l, &p0r, &p0b, &p1u, &p1d, &p1l, &p1r, &p1b, &s, &r);

    if (result == 12) {
      system_switches = 0xB;
      joystick_movement = 0xFF;
      joystick_button_p0 = true;
      joystick_button_p1 = true;

      if (s == '1') system_switches &= ~0x2;
      if (r == '1') system_switches &= ~0x1;
      tas_system_switches[n] = system_switches;

      if (p0r == '1') joystick_movement &= ~0x80;
      if (p0l == '1') joystick_movement &= ~0x40;
      if (p0d == '1') joystick_movement &= ~0x20;
      if (p0u == '1') joystick_movement &= ~0x10;
      if (p1r == '1') joystick_movement &= ~0x08;
      if (p1l == '1') joystick_movement &= ~0x04;
      if (p1d == '1') joystick_movement &= ~0x02;
      if (p1u == '1') joystick_movement &= ~0x01;
      tas_joystick_movement[n] = joystick_movement;

      if (p0b == '1') joystick_button_p0 = false;
      tas_joystick_button_p0[n] = joystick_button_p0;
      if (p1b == '1') joystick_button_p1 = false;
      tas_joystick_button_p1[n] = joystick_button_p1;

      n++;
      if (n >= TAS_DATA_MAX) {
        fprintf(stderr, "Overflow in TAS data.\n");
        return -1;
      }
    }
  }
  tas_data_end = n;

  fclose(fh);
  return 0;
}



int tas_init(const char *filename);
uint8_t tas_get_system_switches(void)
{
  if (tas_active) {
    return tas_system_switches[tas_data_index];
  } else {
    return 0xB;
  }
}



uint8_t tas_get_joystick_movement(void)
{
  if (tas_active) {
    return tas_joystick_movement[tas_data_index];
  } else {
    return 0xFF;
  }
}



bool tas_get_joystick_button_p0(void)
{
  if (tas_active) {
    return tas_joystick_button_p0[tas_data_index];
  } else {
    return true;
  }
}



bool tas_get_joystick_button_p1(void)
{
  if (tas_active) {
    return tas_joystick_button_p1[tas_data_index];
  } else {
    return true;
  }
}



void tas_update(void)
{
  if (! tas_active) {
    return;
  }

  tas_data_index++;

  if (tas_data_index >= TAS_DATA_MAX ||
      tas_data_index >= tas_data_end) {
    tas_data_index = 0;
    tas_active = false;
  }
}



bool tas_is_active(void)
{
  return tas_active;
}



