#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "tia.h"
#include "mem.h"
#include "gui.h"
#include "console.h"
#include "tas.h"
#include "audio.h"
#include "main.h"

#define TIA_DOT_VISIBLE 68
#define TIA_DOT_MAX 228

#define TIA_SCANLINE_VISIBLE_START 27
#define TIA_SCANLINE_VISIBLE_END 254
#define TIA_SCANLINE_MAX 262



static void tia_object_motion_execute(tia_t *tia)
{
  int i;
  int16_t new_pos;

  for (i = 0; i < TIA_OBJECTS; i++) {
    new_pos = (tia->object[i].pos - (tia->object[i].motion >> 4));
    if (new_pos < 0) {
      new_pos = TIA_SCANLINE_WIDTH + new_pos;
    }
    tia->object[i].pos = new_pos % TIA_SCANLINE_WIDTH;
  }
}



static void tia_object_position_reset(tia_t *tia, tia_object_t object)
{
  uint8_t pos;
  uint8_t dot;

  dot = tia->dot;

  switch (object) {
  case TIA_OBJECT_P0:
  case TIA_OBJECT_P1:
    dot += 4;
    break;

  case TIA_OBJECT_M0:
  case TIA_OBJECT_M1:
  case TIA_OBJECT_BL:
    dot += 3;
    break;

  default:
    break;
  }

  if (dot >= TIA_DOT_VISIBLE) {
    pos = dot - TIA_DOT_VISIBLE;
  } else {
    pos = 0;
  }

  tia->object[object].pos = pos % TIA_SCANLINE_WIDTH;
}



static void tia_object_reset_to_player(tia_t *tia, tia_object_t object)
{
  tia_object_t player = 0;

  if (object == TIA_OBJECT_M0) {
    player = TIA_OBJECT_P0;
  } else if (object == TIA_OBJECT_M1) {
    player = TIA_OBJECT_P1;
  } else {
    return;
  }

  if (tia->object[object].reset) {
    switch (tia->object[player].size) {
    case 0: /* Normal Player */
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
      tia->object[object].pos = tia->object[player].pos + 3;
      break;

    case 5: /* Double Sized Player */
      tia->object[object].pos = tia->object[player].pos + 6;
      break;

    case 7: /* Quad Sized Player */
      tia->object[object].pos = tia->object[player].pos + 10;
      break;

    default:
      break;
    }
  }
}



static void tia_object_shape(tia_t *tia, tia_object_t object, uint8_t value)
{
  switch ((value & 0x30) >> 4) {
  case 0:
    tia->object[object].shape = 0x80;
    break;
  case 1:
    tia->object[object].shape = 0xC0;
    break;
  case 2:
    tia->object[object].shape = 0xF0;
    break;
  case 3:
    tia->object[object].shape = 0xFF;
    break;
  }
}



static uint8_t tia_color(uint8_t value)
{
  int color;
  int luminance;

  luminance = ((value >> 1) & 0b111) << 4;
  color     =   value >> 4;

  return (luminance + color) % 128;
}



static uint8_t tia_read_hook(void *tia, uint16_t address)
{
  address &= 0xF; /* Mirroring */
  sync(); /* Sync CPU and TIA. */

  /* Set the lower unused bits of the collision registers to the address.
     This handles bugs in games that used e.g. '$13' instead of '#$13'. */
  switch (address) {
  case TIA_CXM0P:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_M0_P1] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_M0_P0] << 6)) +
            TIA_CXM0P;

  case TIA_CXM1P:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_M1_P0] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_M1_P1] << 6)) + 
            TIA_CXM1P;

  case TIA_CXP0FB:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_P0_PF] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_P0_BL] << 6)) +
            TIA_CXP0FB;

  case TIA_CXP1FB:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_P1_PF] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_P1_BL] << 6)) +
            TIA_CXP1FB;

  case TIA_CXM0FB:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_M0_PF] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_M0_BL] << 6)) +
            TIA_CXM0FB;

  case TIA_CXM1FB:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_M1_PF] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_M1_BL] << 6)) +
            TIA_CXM1FB;

  case TIA_CXBLPF:
    return (((tia_t *)tia)->collision[TIA_COLLISION_BL_PF] << 7) +
            TIA_CXBLPF;

  case TIA_CXPPMM:
    return ((((tia_t *)tia)->collision[TIA_COLLISION_P0_P1] << 7) +
            (((tia_t *)tia)->collision[TIA_COLLISION_M0_M1] << 6)) +
            TIA_CXPPMM;

  case TIA_INPT0:
  case TIA_INPT1:
  case TIA_INPT2:
  case TIA_INPT3:
  case TIA_INPT4:
  case TIA_INPT5:
    return ((tia_t *)tia)->input[address - 8].state ? 0x80 : 0x00;

  default:
    return 0;
  }
}



static void tia_collision_clear(tia_t *tia)
{
  int i;

  for (i = 0; i < TIA_COLLISIONS; i++) {
    tia->collision[i] = false;
  }
}



static void tia_write_hook(void *tia, uint16_t address, uint8_t value)
{
  address &= 0x3F; /* Mirroring */
  sync(); /* Sync CPU and TIA. */

  switch (address) {
  case TIA_VSYNC:
    ((tia_t *)tia)->vsync = (value >> 1) & 1;
    /* Reset scanline to zero if VSYNC is set active. */
    if (((tia_t *)tia)->vsync) {
      if (! ((tia_t *)tia)->vsync_done) {
        ((tia_t *)tia)->dot = 0;
        ((tia_t *)tia)->scanline = 0;
        ((tia_t *)tia)->vsync_done = true;
        ((tia_t *)tia)->wsync_count = 0;
      }
    } else {
      ((tia_t *)tia)->vsync_done = false;
    }
    break;

  case TIA_VBLANK:
    ((tia_t *)tia)->vblank = (value >> 1) & 1;
    ((tia_t *)tia)->input[0].control = (value >> 7);
    ((tia_t *)tia)->input[1].control = (value >> 7);
    ((tia_t *)tia)->input[2].control = (value >> 7);
    ((tia_t *)tia)->input[3].control = (value >> 7);
    ((tia_t *)tia)->input[4].control = (value >> 6) & 1;
    ((tia_t *)tia)->input[5].control = (value >> 6) & 1;
    break;

  case TIA_WSYNC:
    ((tia_t *)tia)->wsync_count++;
    /* Handle special case where WSYNC is set right when dot wraps to 0.
       Then RDY should not be set so the CPU can continue as normal. */
    if (((tia_t *)tia)->dot != 0) {
      ((tia_t *)tia)->rdy = false;
    }
    break;

  case TIA_RSYNC:
    /* Not implemented. */
    break;

  case TIA_NUSIZ0:
    tia_object_shape((tia_t *)tia, TIA_OBJECT_M0, value);
    ((tia_t *)tia)->object[TIA_OBJECT_P0].size = value & 0b111;
    ((tia_t *)tia)->object[TIA_OBJECT_M0].size = value & 0b111;
    break;

  case TIA_NUSIZ1:
    tia_object_shape((tia_t *)tia, TIA_OBJECT_M1, value);
    ((tia_t *)tia)->object[TIA_OBJECT_P1].size = value & 0b111;
    ((tia_t *)tia)->object[TIA_OBJECT_M1].size = value & 0b111;
    break;

  case TIA_COLUP0:
    ((tia_t *)tia)->object[TIA_OBJECT_P0].color = tia_color(value);
    ((tia_t *)tia)->object[TIA_OBJECT_M0].color = tia_color(value);
    break;

  case TIA_COLUP1:
    ((tia_t *)tia)->object[TIA_OBJECT_P1].color = tia_color(value);
    ((tia_t *)tia)->object[TIA_OBJECT_M1].color = tia_color(value);
    break;

  case TIA_COLUPF:
    ((tia_t *)tia)->playfield_color = tia_color(value);
    ((tia_t *)tia)->object[TIA_OBJECT_BL].color = tia_color(value);
    break;

  case TIA_COLUBK:
    ((tia_t *)tia)->background_color = tia_color(value);
    break;

  case TIA_CTRLPF:
    ((tia_t *)tia)->playfield_reflect    =  value       & 1;
    ((tia_t *)tia)->playfield_score_mode = (value >> 1) & 1;
    ((tia_t *)tia)->playfield_priority   = (value >> 2) & 1;
    tia_object_shape((tia_t *)tia, TIA_OBJECT_BL, value);
    break;

  case TIA_REFP0:
    ((tia_t *)tia)->object[TIA_OBJECT_P0].reflect = (value >> 3) & 1;
    break;

  case TIA_REFP1:
    ((tia_t *)tia)->object[TIA_OBJECT_P1].reflect = (value >> 3) & 1;
    break;

  case TIA_PF0:
    ((tia_t *)tia)->playfield[0] = value >> 4;
    break;

  case TIA_PF1:
    ((tia_t *)tia)->playfield[1] = value;
    break;

  case TIA_PF2:
    ((tia_t *)tia)->playfield[2] = value;
    break;

  case TIA_RESP0:
    tia_object_position_reset((tia_t *)tia, TIA_OBJECT_P0);
    break;

  case TIA_RESP1:
    tia_object_position_reset((tia_t *)tia, TIA_OBJECT_P1);
    break;

  case TIA_RESM0:
    tia_object_position_reset((tia_t *)tia, TIA_OBJECT_M0);
    break;

  case TIA_RESM1:
    tia_object_position_reset((tia_t *)tia, TIA_OBJECT_M1);
    break;

  case TIA_RESBL:
    tia_object_position_reset((tia_t *)tia, TIA_OBJECT_BL);
    break;

  case TIA_AUDC0:
    audio_set_control(0, value & 0xF);
    break;

  case TIA_AUDC1:
    audio_set_control(1, value & 0xF);
    break;

  case TIA_AUDF0:
    audio_set_frequency(0, value & 0x1F);
    break;

  case TIA_AUDF1:
    audio_set_frequency(1, value & 0x1F);
    break;

  case TIA_AUDV0:
    audio_set_volume(0, value & 0xF);
    break;

  case TIA_AUDV1:
    audio_set_volume(1, value & 0xF);
    break;

  case TIA_GRP0:
    if (((tia_t *)tia)->object[TIA_OBJECT_P0].vdelay) {
      ((tia_t *)tia)->object[TIA_OBJECT_P0].vdata = value;
    } else {
      ((tia_t *)tia)->object[TIA_OBJECT_P0].enabled = (value > 0);
      ((tia_t *)tia)->object[TIA_OBJECT_P0].shape   = value;
    }
    /* Set pending data due to vertical delay: */
    if (((tia_t *)tia)->object[TIA_OBJECT_P1].vdelay) {
      value = ((tia_t *)tia)->object[TIA_OBJECT_P1].vdata;
      ((tia_t *)tia)->object[TIA_OBJECT_P1].enabled = (value > 0);
      ((tia_t *)tia)->object[TIA_OBJECT_P1].shape   = value;
    }
    break;

  case TIA_GRP1:
    if (((tia_t *)tia)->object[TIA_OBJECT_P1].vdelay) {
      ((tia_t *)tia)->object[TIA_OBJECT_P1].vdata = value;
    } else {
      ((tia_t *)tia)->object[TIA_OBJECT_P1].enabled = (value > 0);
      ((tia_t *)tia)->object[TIA_OBJECT_P1].shape   = value;
    }
    /* Set pending data due to vertical delay: */
    if (((tia_t *)tia)->object[TIA_OBJECT_P0].vdelay) {
      value = ((tia_t *)tia)->object[TIA_OBJECT_P0].vdata;
      ((tia_t *)tia)->object[TIA_OBJECT_P0].enabled = (value > 0);
      ((tia_t *)tia)->object[TIA_OBJECT_P0].shape   = value;
    }
    if (((tia_t *)tia)->object[TIA_OBJECT_BL].vdelay) {
      value = ((tia_t *)tia)->object[TIA_OBJECT_BL].vdata;
      ((tia_t *)tia)->object[TIA_OBJECT_BL].enabled = (value >> 1) & 1;
    }
    break;

  case TIA_ENAM0:
    ((tia_t *)tia)->object[TIA_OBJECT_M0].enabled = (value >> 1) & 1;
    break;

  case TIA_ENAM1:
    ((tia_t *)tia)->object[TIA_OBJECT_M1].enabled = (value >> 1) & 1;
    break;

  case TIA_ENABL:
    if (((tia_t *)tia)->object[TIA_OBJECT_BL].vdelay) {
      ((tia_t *)tia)->object[TIA_OBJECT_BL].vdata = value;
    } else {
      ((tia_t *)tia)->object[TIA_OBJECT_BL].enabled = (value >> 1) & 1;
    }
    break;

  case TIA_HMP0:
    ((tia_t *)tia)->object[TIA_OBJECT_P0].motion = value;
    break;

  case TIA_HMP1:
    ((tia_t *)tia)->object[TIA_OBJECT_P1].motion = value;
    break;

  case TIA_HMM0:
    ((tia_t *)tia)->object[TIA_OBJECT_M0].motion = value;
    break;

  case TIA_HMM1:
    ((tia_t *)tia)->object[TIA_OBJECT_M1].motion = value;
    break;

  case TIA_HMBL:
    ((tia_t *)tia)->object[TIA_OBJECT_BL].motion = value;
    break;

  case TIA_VDELP0:
    ((tia_t *)tia)->object[TIA_OBJECT_P0].vdelay = value & 1;
    break;

  case TIA_VDELP1:
    ((tia_t *)tia)->object[TIA_OBJECT_P1].vdelay = value & 1;
    break;

  case TIA_VDELBL:
    ((tia_t *)tia)->object[TIA_OBJECT_BL].vdelay = value & 1;
    break;

  case TIA_RESMP0:
    ((tia_t *)tia)->object[TIA_OBJECT_M0].reset = (value >> 1) & 1;
    tia_object_reset_to_player((tia_t *)tia, TIA_OBJECT_M0);
    break;

  case TIA_RESMP1:
    ((tia_t *)tia)->object[TIA_OBJECT_M1].reset = (value >> 1) & 1;
    tia_object_reset_to_player((tia_t *)tia, TIA_OBJECT_M1);
    break;

  case TIA_HMOVE:
    tia_object_motion_execute((tia_t *)tia);
    ((tia_t *)tia)->hmove_executed = true;
    break;

  case TIA_HMCLR:
    ((tia_t *)tia)->object[TIA_OBJECT_P0].motion = 0;
    ((tia_t *)tia)->object[TIA_OBJECT_P1].motion = 0;
    ((tia_t *)tia)->object[TIA_OBJECT_M0].motion = 0;
    ((tia_t *)tia)->object[TIA_OBJECT_M1].motion = 0;
    ((tia_t *)tia)->object[TIA_OBJECT_BL].motion = 0;
    break;

  case TIA_CXCLR:
    tia_collision_clear((tia_t *)tia);
    break;

  default:
    break;
  }
}



void tia_init(tia_t *tia, mem_t *mem)
{
  int i;
  mem->tia = tia;
  mem->tia_read  = tia_read_hook;
  mem->tia_write = tia_write_hook;

  tia->dot = 0;
  tia->scanline = 0;

  tia->rdy        = true; /* RDY signal to halt CPU. */
  tia->vsync      = false;
  tia->vsync_done = false;
  tia->vblank     = false;

  tia->hmove_executed = false;
  tia->wsync_count = 0;

  for (i = 0; i < TIA_INPUTS; i++) {
    tia->input[i].state   = false;
    tia->input[i].control = false;
  }

  for (i = 0; i < TIA_OBJECTS; i++) {
    tia->object[i].enabled = false;
    tia->object[i].pos     = 0;
    tia->object[i].shape   = 0;
    tia->object[i].motion  = 0;
    tia->object[i].reflect = false;
    tia->object[i].reset   = false;
    tia->object[i].color   = 0x7F;
    tia->object[i].size    = 0;
    tia->object[i].vdelay  = false;
    tia->object[i].vdata   = 0;
  }

  tia->playfield[0] = 0;
  tia->playfield[1] = 0;
  tia->playfield[2] = 0;
  tia->playfield_reflect    = false;
  tia->playfield_score_mode = false;
  tia->playfield_priority   = false;
  tia->playfield_color  = 0;
  tia->background_color = 0;

  tia_collision_clear(tia);
}



static inline void tia_collision_playfield(tia_t *tia, tia_object_t object)
{
  switch (object) {
  case TIA_OBJECT_P0:
    tia->collision[TIA_COLLISION_P0_PF] = true;
    break;

  case TIA_OBJECT_P1:
    tia->collision[TIA_COLLISION_P1_PF] = true;
    break;

  case TIA_OBJECT_M0:
    tia->collision[TIA_COLLISION_M0_PF] = true;
    break;

  case TIA_OBJECT_M1:
    tia->collision[TIA_COLLISION_M1_PF] = true;
    break;

  case TIA_OBJECT_BL:
    tia->collision[TIA_COLLISION_BL_PF] = true;
    break;

  default:
    break;
  }
}



static inline void tia_collision_object(tia_t *tia, tia_object_t object,
  bool drawn[])
{
  switch (object) {
  case TIA_OBJECT_P0:
    if (drawn[TIA_OBJECT_P1]) {
      tia->collision[TIA_COLLISION_P0_P1] = true;
    }
    if (drawn[TIA_OBJECT_M0]) {
      tia->collision[TIA_COLLISION_M0_P0] = true;
    }
    if (drawn[TIA_OBJECT_M1]) {
      tia->collision[TIA_COLLISION_M1_P0] = true;
    }
    if (drawn[TIA_OBJECT_BL]) {
      tia->collision[TIA_COLLISION_P0_BL] = true;
    }
    break;

  case TIA_OBJECT_P1:
    if (drawn[TIA_OBJECT_P0]) {
      tia->collision[TIA_COLLISION_P0_P1] = true;
    }
    if (drawn[TIA_OBJECT_M0]) {
      tia->collision[TIA_COLLISION_M0_P1] = true;
    }
    if (drawn[TIA_OBJECT_M1]) {
      tia->collision[TIA_COLLISION_M1_P1] = true;
    }
    if (drawn[TIA_OBJECT_BL]) {
      tia->collision[TIA_COLLISION_P1_BL] = true;
    }
    break;

  case TIA_OBJECT_M0:
    if (drawn[TIA_OBJECT_P0]) {
      tia->collision[TIA_COLLISION_M0_P0] = true;
    }
    if (drawn[TIA_OBJECT_P1]) {
      tia->collision[TIA_COLLISION_M0_P1] = true;
    }
    if (drawn[TIA_OBJECT_M1]) {
      tia->collision[TIA_COLLISION_M0_M1] = true;
    }
    if (drawn[TIA_OBJECT_BL]) {
      tia->collision[TIA_COLLISION_M0_BL] = true;
    }
    break;

  case TIA_OBJECT_M1:
    if (drawn[TIA_OBJECT_P0]) {
      tia->collision[TIA_COLLISION_M1_P0] = true;
    }
    if (drawn[TIA_OBJECT_P1]) {
      tia->collision[TIA_COLLISION_M1_P1] = true;
    }
    if (drawn[TIA_OBJECT_M0]) {
      tia->collision[TIA_COLLISION_M0_M1] = true;
    }
    if (drawn[TIA_OBJECT_BL]) {
      tia->collision[TIA_COLLISION_M1_BL] = true;
    }
    break;

  case TIA_OBJECT_BL:
    if (drawn[TIA_OBJECT_P0]) {
      tia->collision[TIA_COLLISION_P0_BL] = true;
    }
    if (drawn[TIA_OBJECT_P1]) {
      tia->collision[TIA_COLLISION_P1_BL] = true;
    }
    if (drawn[TIA_OBJECT_M0]) {
      tia->collision[TIA_COLLISION_M0_BL] = true;
    }
    if (drawn[TIA_OBJECT_M1]) {
      tia->collision[TIA_COLLISION_M1_BL] = true;
    }
    break;

  default:
    break;
  }
}



static bool tia_object_dot(tia_t *tia, tia_object_t object,
  int pos, int offset, int expand)
{
  int i, j, dot;

  for (i = 0; i < 8; i++) {
    if (tia->object[object].reflect) {
      dot = ((tia->object[object].pos + (i * expand)) + 1);
      dot = (dot + offset);
    } else {
      dot = ((tia->object[object].pos - (i * expand)) + (8 * expand));
      dot = (dot + offset);
    }

    if (expand > 1) {
      dot += 1; /* Position gets shifted on large players. */
    }

    if ((tia->object[object].shape >> i) & 1) {
      for (j = 0; j < expand; j++) {
        if (tia->object[object].reflect) {
          if (((dot + j) % TIA_SCANLINE_WIDTH) == pos) {
            return true;
          }
        } else {
          if (((dot - j) % TIA_SCANLINE_WIDTH) == pos) {
            return true;
          }
        }
      }
    }
  }

  return false;
}



static bool tia_object_active(tia_t *tia, tia_object_t object, int pos)
{
  switch (tia->object[object].size) {
  case 0: /* One Copy */
    return tia_object_dot(tia, object, pos, 0, 1);

  case 1: /* Two Copies, Close Distance */
    return tia_object_dot(tia, object, pos,  0, 1) |
           tia_object_dot(tia, object, pos, 16, 1);

  case 2: /* Two Copies, Medium Distance */
    return tia_object_dot(tia, object, pos,  0, 1) |
           tia_object_dot(tia, object, pos, 32, 1);

  case 3: /* Three Copies, Close Distance */
    return tia_object_dot(tia, object, pos,  0, 1) |
           tia_object_dot(tia, object, pos, 16, 1) |
           tia_object_dot(tia, object, pos, 32, 1);

  case 4: /* Two Copies, Wide Distance */
    return tia_object_dot(tia, object, pos,  0, 1) |
           tia_object_dot(tia, object, pos, 64, 1);

  case 5: /* Double Sized Player */
    if ((object == TIA_OBJECT_P0) || (object == TIA_OBJECT_P1)) {
      return tia_object_dot(tia, object, pos, 0, 2);
    } else {
      return tia_object_dot(tia, object, pos, 0, 1);
    }

  case 6: /* Three Copies, Medium Distance */
    return tia_object_dot(tia, object, pos,  0, 1) |
           tia_object_dot(tia, object, pos, 32, 1) |
           tia_object_dot(tia, object, pos, 64, 1);

  case 7: /* Quad Sized Player */
    if ((object == TIA_OBJECT_P0) || (object == TIA_OBJECT_P1)) {
      return tia_object_dot(tia, object, pos, 0, 4);
    } else {
      return tia_object_dot(tia, object, pos, 0, 1);
    }
    break;

  default:
    break;
  }

  return false;
}



static bool tia_playfield_active(tia_t *tia, int pos)
{
  pos /= 4; /* Sections are minimum 4 pixels wide. */

  if (pos < 4) { /* First Half */
    return ((tia->playfield[0] >> pos) & 1);
  } else if (pos < 12) {
    return ((tia->playfield[1] >> (7 - (pos - 4))) & 1);
  } else if (pos < 20) {
    return ((tia->playfield[2] >> (pos - 12)) & 1);

  } else if (tia->playfield_reflect) { /* Second Half, Reflected */
    if (pos < 28) {
      return ((tia->playfield[2] >> (7 - (pos - 20))) & 1);
    } else if (pos < 36) {
      return ((tia->playfield[1] >> (pos - 28)) & 1);
    } else if (pos < 40) {
      return ((tia->playfield[0] >> (3 - (pos - 36))) & 1);
    }

  } else { /* Second Half, Normal */
    if (pos < 24) {
      return ((tia->playfield[0] >> (pos - 20)) & 1);
    } else if (pos < 32) {
      return ((tia->playfield[1] >> (7 - (pos - 24))) & 1);
    } else if (pos < 40) {
      return ((tia->playfield[2] >> (pos - 32)) & 1);
    }
  }

  return false;
}



static uint8_t tia_playfield_color(tia_t *tia, int pos)
{
  if (pos < 80) { /* First Half */
    if (tia->playfield_score_mode && (! tia->playfield_priority)) {
      return tia->object[TIA_OBJECT_P0].color;
    } else {
      return tia->playfield_color;
    }
  } else { /* Second Half */
    if (tia->playfield_score_mode && (! tia->playfield_priority)) {
      return tia->object[TIA_OBJECT_P1].color;
    } else {
      return tia->playfield_color;
    }
  }
}



static void tia_draw_dot(tia_t *tia)
{
  int i;
  int pos = tia->dot - TIA_DOT_VISIBLE;
  bool playfield_drawn = false;
  bool object_drawn[TIA_OBJECTS] = {false, false, false, false, false};

  /* Draw black if VBLANK is active: */
  if (tia->vblank) {
    tia->scanline_colors[pos] = 0;
    tia->scanline_object[pos] = TIA_OBJECT_VB;
    return;
  }

  /* Draw black on first 8 pixels if HMOVE was executed: */
  if (tia->hmove_executed && pos < 8) {
    tia->scanline_colors[pos] = 0;
    tia->scanline_object[pos] = TIA_OBJECT_HM;
    return;
  }

  /* Draw playfield/background: */
  if (tia_playfield_active(tia, pos)) {
    tia->scanline_colors[pos] = tia_playfield_color(tia, pos);
    tia->scanline_object[pos] = TIA_OBJECT_PF;
    playfield_drawn = true;
  } else {
    tia->scanline_colors[pos] = tia->background_color;
    tia->scanline_object[pos] = TIA_OBJECT_BK;
  }

  /* Draw objects: */
  for (i = 0; i < TIA_OBJECTS; i++) {
    if (tia->object[i].enabled && !tia->object[i].reset) {
      if (tia_object_active(tia, i, pos)) {
        tia->scanline_colors[pos] = tia->object[i].color;
        tia->scanline_object[pos] = i;

        /* Collision detection: */
        if (playfield_drawn) {
          tia_collision_playfield(tia, i);
        }
        tia_collision_object(tia, i, object_drawn);
        object_drawn[i] = true;
      }
    }
  }

  /* Redraw playfield on top if it has priority: */
  if (tia->playfield_priority && tia_playfield_active(tia, pos)) {
    tia->scanline_colors[pos] = tia_playfield_color(tia, pos);
    tia->scanline_object[pos] = TIA_OBJECT_PF;
  }
}



void tia_execute(tia_t *tia)
{
  bool visible_scanline = tia->scanline >= TIA_SCANLINE_VISIBLE_START &&
                          tia->scanline <= TIA_SCANLINE_VISIBLE_END;

  tia->dot++;
  if (tia->dot >= TIA_DOT_MAX) {
    tia->dot = 0;
    tia->rdy = true;

    if (visible_scanline) {
      gui_draw_scanline(tia->scanline - TIA_SCANLINE_VISIBLE_START,
        tia->scanline_colors);
      console_draw_scanline(tia->scanline - TIA_SCANLINE_VISIBLE_START,
        tia->scanline_colors, tia->scanline_object);
    }
    tia->hmove_executed = false;

    tia->scanline++;
    if (tia->scanline >= TIA_SCANLINE_MAX) {
      tia->scanline = 0;
    }

  } else if (tia->dot >= TIA_DOT_VISIBLE) {
    if (visible_scanline) {
      tia_draw_dot(tia);
    }
  }

  if (tas_is_active()) {
    tia->input[4].state = tas_get_joystick_button_p0();
    tia->input[5].state = tas_get_joystick_button_p1();
  } else {
    tia->input[4].state = gui_get_joystick_button_p0() &
                          console_get_joystick_button_p0();
    tia->input[5].state = gui_get_joystick_button_p1() &
                          console_get_joystick_button_p1();
  }
}



void tia_dump(FILE *fh, tia_t *tia)
{
  int i;
  static const char *object_name[TIA_OBJECTS] = {"P0","P1","M0","M1","BL"};
  static const char *collision_name[TIA_COLLISIONS] =
    {"M0-P1", "M0-P0", "M1-P0", "M1-P1", "P0-PF", "P0-BL", "P1-PF", "P1-BL",
     "M0-PF", "M0-BL", "M1-PF", "M1-BL", "BL-PF", "P0-P1", "M0-M1"};

  fprintf(fh, "Scanline/Dot: %d/%d\n", tia->scanline, tia->dot);
  fprintf(fh, "RDY   : %d (%s)\n", tia->rdy, tia->rdy ? "Run" : "Halt");
  fprintf(fh, "VSYNC : %d (Done: %d)\n", tia->vsync, tia->vsync_done);
  fprintf(fh, "VBLANK: %d\n", tia->vblank);
  fprintf(fh, "WSYNC : %d times\n", tia->wsync_count);

  for (i = 0; i < TIA_OBJECTS; i++) {
    fprintf(fh, "Object '%s':\n", object_name[i]);
    fprintf(fh, "  Enabled : %d\n", tia->object[i].enabled);
    fprintf(fh, "  Position: %d\n", tia->object[i].pos);
    fprintf(fh, "  Shape   : 0x%02x\n", tia->object[i].shape);
    fprintf(fh, "  Motion  : %d\n", tia->object[i].motion >> 4);
    fprintf(fh, "  Reflect : %d\n", tia->object[i].reflect);
    fprintf(fh, "  Reset   : %d\n", tia->object[i].reset);
    fprintf(fh, "  Color   : %d\n", tia->object[i].color);
    fprintf(fh, "  Size    : 0x%02x\n", tia->object[i].size);
    fprintf(fh, "  Vertical:\n");
    fprintf(fh, "    Delay : %d\n", tia->object[i].vdelay);
    fprintf(fh, "    Data  : 0x%02x\n", tia->object[i].vdata);
  }

  fprintf(fh, "Playfield: %02x:%02x:%02x\n",
    tia->playfield[0], tia->playfield[1], tia->playfield[2]);
  fprintf(fh, "  Reflect         : %d\n", tia->playfield_reflect);
  fprintf(fh, "  Score Mode      : %d\n", tia->playfield_score_mode);
  fprintf(fh, "  Color           : %d\n", tia->playfield_color);
  fprintf(fh, "  Background Color: %d\n", tia->background_color);

  fprintf(fh, "Collisions:\n");
  for (i = 0; i < TIA_COLLISIONS; i++) {
    fprintf(fh, "  '%s': %d\n", collision_name[i], tia->collision[i]);
  }

  for (i = 0; i < 6; i++) {
    fprintf(fh, "Input #%d\n", i);
    fprintf(fh, "  State  : %d\n", tia->input[i].state);
    fprintf(fh, "  Control: %d\n", tia->input[i].control);
  }
}



