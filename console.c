#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>

#include "console.h"
#include "tia.h"



static bool console_active = false;
static bool console_colors = false;
static int console_maxy;
static int console_maxx;
static bool console_vblank_strip = false;

static const uint16_t console_system_switches_map[5] =
  {0x1, 0x2, 0x8, 0x40, 0x80};
static const uint16_t console_joystick_movement_map[4] =
  {0x10, 0x20, 0x40, 0x80};

static uint8_t console_system_switches = 0xB;
static uint8_t console_joystick_movement = 0xFF;
static bool console_joystick_button_p0 = true;
static bool console_joystick_button_p1 = true;



/* Using colors from the default rxvt color palette. */
static const int console_color_map[128] = {
  232,  58,  52,  88,  88,  89,  54,  18,
   18,  18,  17,  22,  22,  22,  22,  52,
  238,  58,  94,  94, 124,  90,  54,  54,
   19,  24,  24,  23,  22,  58,  58,  58,
  242, 100,  94, 131, 131, 132,  97,  61,
   61,  61,  60,  65,  65,  65,  59,  95,
  246, 143, 137, 131, 131, 133,  97,  61,
   61,  61,  67,  72,  71,  65, 101, 137,
  249, 143, 137, 173, 167, 133, 134,  98,
   68,  68,  73,  72, 108, 107, 143, 143,
  251, 185, 179, 173, 174, 175, 140, 104,
  104, 110, 110, 115, 114, 150, 144, 179,
  253, 185, 179, 216, 217, 182, 183, 147,
  111, 111, 116, 115, 151, 150, 186, 186,
  255, 227, 216, 217, 218, 183, 147, 147,
  153, 153, 158, 157, 193, 193, 222, 231,
};



void console_pause(void)
{
  if (! console_active) {
    return;
  }

  endwin();
  timeout(-1);
}



void console_resume(void)
{
  if (! console_active) {
    return;
  }

  timeout(0);
  refresh();
}



void console_exit(void)
{
  curs_set(1);
  endwin();
}



void console_init(bool vblank_strip, bool disable_colors)
{
  int i;

  console_active = true;
  console_vblank_strip = vblank_strip;

  initscr();
  atexit(console_exit);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0); /* Non-blocking mode. */
  curs_set(0); /* Hide cursor. */

  getmaxyx(stdscr, console_maxy, console_maxx);

  if (disable_colors) {
    console_colors = false;
  } else {
    console_colors = true;

    if (has_colors() && can_change_color()) {
      start_color();
      use_default_colors();

      for (i = 0; i < 128; i++) {
        init_pair(i + 1, 0, console_color_map[i]);
      }
    }
  }
}



static void console_winch(void)
{
  getmaxyx(stdscr, console_maxy, console_maxx);
  flushinp();
  keypad(stdscr, TRUE);
}



uint8_t console_get_system_switches(void)
{
  return console_system_switches;
}



uint8_t console_get_joystick_movement(void)
{
  return console_joystick_movement;
}



bool console_get_joystick_button_p0(void)
{
  return console_joystick_button_p0;
}



bool console_get_joystick_button_p1(void)
{
  return console_joystick_button_p1;
}



void console_update(void)
{
  static int cycle = 0;
  int c;

  if (! console_active) {
    return;
  }

  /* Check for keyboard input: */
  cycle++;
  if (cycle % 2 == 0) {
    c = getch();
    switch (c) {
    case ERR: /* All keys released. */
      console_joystick_button_p0 = true;
      console_joystick_movement |= console_joystick_movement_map[0];
      console_joystick_movement |= console_joystick_movement_map[1];
      console_joystick_movement |= console_joystick_movement_map[2];
      console_joystick_movement |= console_joystick_movement_map[3];
      console_system_switches |= console_system_switches_map[0];
      console_system_switches |= console_system_switches_map[1];
      break;

    case KEY_RESIZE:
      console_winch();
      break;

    case ' ':
      console_joystick_button_p0 = false;
      break;

    case KEY_UP:
      console_joystick_movement &= ~console_joystick_movement_map[0];
      break;

    case KEY_DOWN:
      console_joystick_movement &= ~console_joystick_movement_map[1];
      break;

    case KEY_LEFT:
      console_joystick_movement &= ~console_joystick_movement_map[2];
      break;

    case KEY_RIGHT:
      console_joystick_movement &= ~console_joystick_movement_map[3];
      break;

    case 'z': /* Reset */
      console_system_switches &= ~console_system_switches_map[0];
      break;

    case 'x': /* Select */
      console_system_switches &= ~console_system_switches_map[1];
      break;

    case 'c': /* Color */
      console_system_switches ^= console_system_switches_map[2];
      break;

    case '1': /* Player 0 Difficulty */
      console_system_switches ^= console_system_switches_map[3];
      break;

    case '2': /* Player 1 Difficulty */
      console_system_switches ^= console_system_switches_map[4];
      break;

    case 'Q':
    case 'q':
      exit(EXIT_SUCCESS);
      break;

    default:
      break;
    }
  }

  /* Update screen: */
  refresh();
}



void console_draw_scanline(uint16_t y, uint8_t colors[], tia_object_t object[])
{
  static int vblank_start = 0;
  static int vblank_end = 228;
  int con_y;
  int con_x;
  int pos;
  uint8_t color_pair;

  if (! console_active) {
    return;
  }

  if (console_vblank_strip) {
    /* Strip VBLANK from top/bottom to save precious console real estate. */
    con_y = (int)((y - vblank_start) / 
      ((vblank_end - vblank_start) / (float)console_maxy));
  } else {
    con_y = (int)(y / (228 / (float)console_maxy));
  }

  for (con_x = 0; con_x < console_maxx; con_x++) {
    pos = (int)(con_x / (console_maxx / 160.0));

    if (console_colors) {
      color_pair = colors[pos] + 1;
      attron(COLOR_PAIR(color_pair));
    }

    switch (object[pos]) {
    case TIA_OBJECT_PF:
      mvaddch(con_y, con_x, '#');
      break;

    case TIA_OBJECT_VB:
      if (console_vblank_strip) {
        /* Learn the start and end of VBLANK for automatic adjustment. */
        if (y < 50 && y > vblank_start) {
          vblank_start = y;
        } else if (y > 150 && y < vblank_end) {
          vblank_end = y;
        }
      } else {
        mvaddch(con_y, con_x, ' ');
      }
      break;

    case TIA_OBJECT_BK:
    case TIA_OBJECT_HM:
      mvaddch(con_y, con_x, ' ');
      break;

    case TIA_OBJECT_P0:
      mvaddch(con_y, con_x, '1');
      break;

    case TIA_OBJECT_P1:
      mvaddch(con_y, con_x, '2');
      break;

    case TIA_OBJECT_M0:
    case TIA_OBJECT_M1:
      mvaddch(con_y, con_x, '%');
      break;

    case TIA_OBJECT_BL:
      mvaddch(con_y, con_x, '*');
      break;

    default:
      break;
    }

    if (console_colors) {
      attroff(COLOR_PAIR(color_pair));
    }
  }
}



