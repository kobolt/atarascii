#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <time.h>

#include "audio.h"

#define GUI_WIDTH 160
#define GUI_HEIGHT (192 + 36) /* Include 36 vblank and overscan lines. */
#define GUI_W_SCALE 6
#define GUI_H_SCALE 3



static SDL_Window *gui_window = NULL;
static SDL_Renderer *gui_renderer = NULL;
static SDL_Texture *gui_texture = NULL;
static SDL_Joystick *gui_joystick = NULL;
static SDL_PixelFormat *gui_pixel_format = NULL;
static Uint32 *gui_pixels = NULL;
static int gui_pixel_pitch = 0;
static Uint32 gui_ticks = 0;

static uint8_t gui_system_switches = 0xB;
static uint8_t gui_joystick_movement = 0xFF;
static bool gui_joystick_button_p0 = true;
static bool gui_joystick_button_p1 = true;
static bool gui_save_state_request = false;
static bool gui_load_state_request = false;

static const uint8_t gui_sys_palette[128][3] =
{
  {0x00, 0x00, 0x00},
  {0x44, 0x44, 0x00},
  {0x70, 0x28, 0x00},
  {0x84, 0x18, 0x00},
  {0x88, 0x00, 0x00},
  {0x78, 0x00, 0x5c},
  {0x48, 0x00, 0x78},
  {0x14, 0x00, 0x84},
  {0x00, 0x00, 0x88},
  {0x00, 0x18, 0x7c},
  {0x00, 0x2c, 0x5c},
  {0x00, 0x40, 0x2c},
  {0x00, 0x3c, 0x00},
  {0x14, 0x38, 0x00},
  {0x2c, 0x30, 0x00},
  {0x44, 0x28, 0x00},
  {0x40, 0x40, 0x40},
  {0x64, 0x64, 0x10},
  {0x84, 0x44, 0x14},
  {0x98, 0x34, 0x18},
  {0x9c, 0x20, 0x20},
  {0x8c, 0x20, 0x74},
  {0x60, 0x20, 0x90},
  {0x30, 0x20, 0x98},
  {0x1c, 0x20, 0x9c},
  {0x1c, 0x38, 0x90},
  {0x1c, 0x4c, 0x78},
  {0x1c, 0x5c, 0x48},
  {0x20, 0x5c, 0x20},
  {0x34, 0x5c, 0x1c},
  {0x4c, 0x50, 0x1c},
  {0x64, 0x48, 0x18},
  {0x6c, 0x6c, 0x6c},
  {0x84, 0x84, 0x24},
  {0x98, 0x5c, 0x28},
  {0xac, 0x50, 0x30},
  {0xb0, 0x3c, 0x3c},
  {0xa0, 0x3c, 0x88},
  {0x78, 0x3c, 0xa4},
  {0x4c, 0x3c, 0xac},
  {0x38, 0x40, 0xb0},
  {0x38, 0x54, 0xa8},
  {0x38, 0x68, 0x90},
  {0x38, 0x7c, 0x64},
  {0x40, 0x7c, 0x40},
  {0x50, 0x7c, 0x38},
  {0x68, 0x70, 0x34},
  {0x84, 0x68, 0x30},
  {0x90, 0x90, 0x90},
  {0xa0, 0xa0, 0x34},
  {0xac, 0x78, 0x3c},
  {0xc0, 0x68, 0x48},
  {0xc0, 0x58, 0x58},
  {0xb0, 0x58, 0x9c},
  {0x8c, 0x58, 0xb8},
  {0x68, 0x58, 0xc0},
  {0x50, 0x5c, 0xc0},
  {0x50, 0x70, 0xbc},
  {0x50, 0x84, 0xac},
  {0x50, 0x9c, 0x80},
  {0x5c, 0x9c, 0x5c},
  {0x6c, 0x98, 0x50},
  {0x84, 0x8c, 0x4c},
  {0xa0, 0x84, 0x44},
  {0xb0, 0xb0, 0xb0},
  {0xb8, 0xb8, 0x40},
  {0xbc, 0x8c, 0x4c},
  {0xd0, 0x80, 0x5c},
  {0xd0, 0x70, 0x70},
  {0xc0, 0x70, 0xb0},
  {0xa0, 0x70, 0xcc},
  {0x7c, 0x70, 0xd0},
  {0x68, 0x74, 0xd0},
  {0x68, 0x88, 0xcc},
  {0x68, 0x9c, 0xc0},
  {0x68, 0xb4, 0x94},
  {0x74, 0xb4, 0x74},
  {0x84, 0xb4, 0x68},
  {0x9c, 0xa8, 0x64},
  {0xb8, 0x9c, 0x58},
  {0xc8, 0xc8, 0xc8},
  {0xd0, 0xd0, 0x50},
  {0xcc, 0xa0, 0x5c},
  {0xe0, 0x94, 0x70},
  {0xe0, 0x88, 0x88},
  {0xd0, 0x84, 0xc0},
  {0xb4, 0x84, 0xdc},
  {0x94, 0x88, 0xe0},
  {0x7c, 0x8c, 0xe0},
  {0x7c, 0x9c, 0xdc},
  {0x7c, 0xb4, 0xd4},
  {0x7c, 0xd0, 0xac},
  {0x8c, 0xd0, 0x8c},
  {0x9c, 0xcc, 0x7c},
  {0xb4, 0xc0, 0x78},
  {0xd0, 0xb4, 0x6c},
  {0xdc, 0xdc, 0xdc},
  {0xe8, 0xe8, 0x5c},
  {0xdc, 0xb4, 0x68},
  {0xec, 0xa8, 0x80},
  {0xec, 0xa0, 0xa0},
  {0xdc, 0x9c, 0xd0},
  {0xc4, 0x9c, 0xec},
  {0xa8, 0xa0, 0xec},
  {0x90, 0xa4, 0xec},
  {0x90, 0xb4, 0xec},
  {0x90, 0xcc, 0xe8},
  {0x90, 0xe4, 0xc0},
  {0xa4, 0xe4, 0xa4},
  {0xb4, 0xe4, 0x90},
  {0xcc, 0xd4, 0x88},
  {0xe8, 0xcc, 0x7c},
  {0xec, 0xec, 0xec},
  {0xfc, 0xfc, 0x68},
  {0xfc, 0xbc, 0x94},
  {0xfc, 0xb4, 0xb4},
  {0xec, 0xb0, 0xe0},
  {0xd4, 0xb0, 0xfc},
  {0xbc, 0xb4, 0xfc},
  {0xa4, 0xb8, 0xfc},
  {0xa4, 0xc8, 0xfc},
  {0xa4, 0xe0, 0xfc},
  {0xa4, 0xfc, 0xd4},
  {0xb8, 0xfc, 0xb8},
  {0xc8, 0xfc, 0xa4},
  {0xe0, 0xec, 0x9c},
  {0xfc, 0xe0, 0x8c},
  {0xff, 0xff, 0xff},
};



static void gui_exit_handler(void)
{
  audio_exit();

  if (SDL_JoystickGetAttached(gui_joystick)) {
    SDL_JoystickClose(gui_joystick);
  }
  if (gui_pixel_format != NULL) {
    SDL_FreeFormat(gui_pixel_format);
  }
  if (gui_texture != NULL) {
    SDL_UnlockTexture(gui_texture);
    SDL_DestroyTexture(gui_texture);
  }
  if (gui_renderer != NULL) {
    SDL_DestroyRenderer(gui_renderer);
  }
  if (gui_window != NULL) {
    SDL_DestroyWindow(gui_window);
  }
  SDL_Quit();
}



int gui_init(int joystick_no, bool disable_video, bool disable_audio)
{
  Uint32 flags;

  flags = SDL_INIT_JOYSTICK;
  if (! disable_video) {
    flags |= SDL_INIT_VIDEO;
  }
  if (! disable_audio) {
    flags |= SDL_INIT_AUDIO;
  }

  if (SDL_Init(flags) != 0) {
    fprintf(stderr, "Unable to initalize SDL: %s\n", SDL_GetError());
    return -1;
  }
  atexit(gui_exit_handler);

  if (! disable_video) {
    if ((gui_window = SDL_CreateWindow("atarascii",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      GUI_WIDTH * GUI_W_SCALE, GUI_HEIGHT * GUI_H_SCALE, 0)) == NULL) {
      fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
      return -1;
    }

    if ((gui_renderer = SDL_CreateRenderer(gui_window, -1, 0)) == NULL) {
      fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
      return -1;
    }

    if ((gui_texture = SDL_CreateTexture(gui_renderer, 
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
      GUI_WIDTH * GUI_W_SCALE, GUI_HEIGHT * GUI_H_SCALE)) == NULL) {
      fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
      return -1;
    }

    if (SDL_LockTexture(gui_texture, NULL,
      (void **)&gui_pixels, &gui_pixel_pitch) != 0) {
      fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
      return -1;
    }

    if ((gui_pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888)) == NULL) {
      fprintf(stderr, "Unable to create pixel format: %s\n", SDL_GetError());
      return -1;
    }
  }

  if (SDL_NumJoysticks() > joystick_no) {
    gui_joystick = SDL_JoystickOpen(joystick_no);
    fprintf(stderr, "Found Joystick: %s\n", SDL_JoystickName(gui_joystick));
  }

  if (disable_audio) {
    return 0;
  } else {
    return audio_init();
  }
}



void gui_draw_scanline(uint16_t y, uint8_t colors[])
{
  int x;
  int scale_x, scale_y;
  int out_x, out_y;

  if (gui_renderer == NULL) {
    return;
  }

  for (x = 0; x < GUI_WIDTH; x++) {
    for (scale_y = 0; scale_y < GUI_H_SCALE; scale_y++) {
      for (scale_x = 0; scale_x < GUI_W_SCALE; scale_x++) {
        out_y = (y * GUI_H_SCALE) + scale_y;
        out_x = (x * GUI_W_SCALE) + scale_x;
        gui_pixels[(out_y * GUI_WIDTH * GUI_W_SCALE) + out_x] = 
          SDL_MapRGB(gui_pixel_format,
          gui_sys_palette[colors[x] % 128][0],
          gui_sys_palette[colors[x] % 128][1],
          gui_sys_palette[colors[x] % 128][2]);
      }
    }
  }
}



uint8_t gui_get_system_switches(void)
{
  return gui_system_switches;
}



uint8_t gui_get_joystick_movement(void)
{
  return gui_joystick_movement;
}



bool gui_get_joystick_button_p0(void)
{
  return gui_joystick_button_p0;
}



bool gui_get_joystick_button_p1(void)
{
  return gui_joystick_button_p1;
}



bool gui_save_state_requested(void)
{
  if (gui_save_state_request) {
    gui_save_state_request = false;
    return true;
  } else {
    return false;
  }
}



bool gui_load_state_requested(void)
{
  if (gui_load_state_request) {
    gui_load_state_request = false;
    return true;
  } else {
    return false;
  }
}



void gui_update(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event) == 1) {
    switch (event.type) {
    case SDL_QUIT:
      exit(EXIT_SUCCESS);
      break;

    /* Keyboard-based Joystick */
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      switch (event.key.keysym.sym) {
      case SDLK_SPACE:
        if (event.type == SDL_KEYDOWN) {
          gui_joystick_button_p0 = false;
        } else {
          gui_joystick_button_p0 = true;
        }
        break;

      case SDLK_UP:
        if (event.type == SDL_KEYDOWN) {
          gui_joystick_movement &= ~0x10;
        } else {
          gui_joystick_movement |= 0x10;
        }
        break;

      case SDLK_DOWN:
        if (event.type == SDL_KEYDOWN) {
          gui_joystick_movement &= ~0x20;
        } else {
          gui_joystick_movement |= 0x20;
        }
        break;

      case SDLK_LEFT:
        if (event.type == SDL_KEYDOWN) {
          gui_joystick_movement &= ~0x40;
        } else {
          gui_joystick_movement |= 0x40;
        }
        break;

      case SDLK_RIGHT:
        if (event.type == SDL_KEYDOWN) {
          gui_joystick_movement &= ~0x80;
        } else {
          gui_joystick_movement |= 0x80;
        }
        break;

      case SDLK_z: /* Reset */
        if (event.type == SDL_KEYDOWN) {
          gui_system_switches &= ~0x1;
        } else {
          gui_system_switches |= 0x1;
        }
        break;

      case SDLK_x: /* Select */
        if (event.type == SDL_KEYDOWN) {
          gui_system_switches &= ~0x2;
        } else {
          gui_system_switches |= 0x2;
        }
        break;

      case SDLK_c: /* Color */
        if (event.type == SDL_KEYDOWN) {
          gui_system_switches ^= 0x8;
        }
        break;

      case SDLK_1: /* Player 0 Difficulty */
        if (event.type == SDL_KEYDOWN) {
          gui_system_switches ^= 0x40;
        }
        break;

      case SDLK_2: /* Player 1 Difficulty */
        if (event.type == SDL_KEYDOWN) {
          gui_system_switches ^= 0x80;
        }
        break;

      case SDLK_F5: /* Save State */
        if (event.type == SDL_KEYDOWN) {
          gui_save_state_request = true;
        }
        break;

      case SDLK_F8: /* Load State */
        if (event.type == SDL_KEYDOWN) {
          gui_load_state_request = true;
        }
        break;

      case SDLK_q: /* Quit */
        if (event.type == SDL_KEYDOWN) {
          exit(EXIT_SUCCESS);
        }
        break;
      }
      break;

    /* Actual Joystick */
    case SDL_JOYAXISMOTION:
      if (event.jaxis.axis == 0) {
        if (event.jaxis.value > 16384) { /* Right Pressed */
          gui_joystick_movement &= ~0x80;
          gui_joystick_movement |=  0x40;
        } else if (event.jaxis.value < -16384) { /* Left Pressed */
          gui_joystick_movement |=  0x80;
          gui_joystick_movement &= ~0x40;
        } else {
          gui_joystick_movement |= 0xC0; /* Right/Left Released */
        }

      } else if (event.jaxis.axis == 1) {
        if (event.jaxis.value > 16384) { /* Down Pressed */
          gui_joystick_movement &= ~0x20;
          gui_joystick_movement |=  0x10;
        } else if (event.jaxis.value < -16384) { /* Up Pressed */
          gui_joystick_movement |=  0x20;
          gui_joystick_movement &= ~0x10;
        } else {
          gui_joystick_movement |= 0x30; /* Down/Up Released */
        }
      }
      break;

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      switch (event.jbutton.button) {
      case 0:
      case 1:
      case 2:
      case 3:
        if (event.jbutton.state == 1) {
          gui_joystick_button_p0 = false;
        } else {
          gui_joystick_button_p0 = true;
        }
        break;

      case 4: /* Save State */
        if (event.jbutton.state == 1) {
          gui_save_state_request = true;
        }
        break;

      case 5: /* Load State */
        if (event.jbutton.state == 1) {
          gui_load_state_request = true;
        }
        break;

      case 6:
        if (event.jbutton.state == 1) { /* Select */
          gui_system_switches &= ~0x2;
        } else {
          gui_system_switches |= 0x2;
        }
        break;

      case 7:
        if (event.jbutton.state == 1) { /* Reset */
          gui_system_switches &= ~0x1;
        } else {
          gui_system_switches |= 0x1;
        }
        break;
      }
      break;
    }
  }

  if (gui_renderer != NULL) {
    SDL_UnlockTexture(gui_texture);

    SDL_RenderCopy(gui_renderer, gui_texture, NULL, NULL);

    if (SDL_LockTexture(gui_texture, NULL,
      (void **)&gui_pixels, &gui_pixel_pitch) != 0) {
      fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }
  }

  /* Force 60 Hz (NTSC) */
  while ((SDL_GetTicks() - gui_ticks) < 16) {
    SDL_Delay(1);
  }

  if (gui_renderer != NULL) {
    SDL_RenderPresent(gui_renderer);
  }

  gui_ticks = SDL_GetTicks();
}



