#ifndef _TIA_H
#define _TIA_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"

#define TIA_SCANLINE_WIDTH 160
#define TIA_INPUTS 6

typedef enum {
  TIA_OBJECT_P0 = 0, /* Player 0 */
  TIA_OBJECT_P1 = 1, /* Player 1 */
  TIA_OBJECT_M0 = 2, /* Missile 0 */
  TIA_OBJECT_M1 = 3, /* Missile 1 */
  TIA_OBJECT_BL = 4, /* Ball */
  TIA_OBJECTS   = 5, /* Total/Limit */
  TIA_OBJECT_PF = 10, /* Playfield (Special) */
  TIA_OBJECT_BK = 11, /* Background (Special) */
  TIA_OBJECT_VB = 12, /* VBLANK (Special */
  TIA_OBJECT_HM = 13, /* HMOVE (Special) */
} tia_object_t;

typedef enum {
  TIA_COLLISION_M0_P1 = 0,
  TIA_COLLISION_M0_P0 = 1,
  TIA_COLLISION_M1_P0 = 2,
  TIA_COLLISION_M1_P1 = 3,
  TIA_COLLISION_P0_PF = 4,
  TIA_COLLISION_P0_BL = 5,
  TIA_COLLISION_P1_PF = 6,
  TIA_COLLISION_P1_BL = 7,
  TIA_COLLISION_M0_PF = 8,
  TIA_COLLISION_M0_BL = 9,
  TIA_COLLISION_M1_PF = 10,
  TIA_COLLISION_M1_BL = 11,
  TIA_COLLISION_BL_PF = 12,
  TIA_COLLISION_P0_P1 = 13,
  TIA_COLLISION_M0_M1 = 14,
  TIA_COLLISIONS      = 15,
} tia_collision_t;

typedef struct tia_object_data_s {
  bool enabled;
  uint8_t pos;
  uint8_t shape;
  int8_t motion; /* Pending motion until HMOVE. */
  bool reflect; /* Mirror the shape. */
  bool reset;
  uint8_t color;
  uint8_t size;
  bool vdelay;
  uint8_t vdata; /* Pending data due to vdelay. */
} tia_object_data_t;

typedef struct tia_input_data_s {
  bool state;
  bool control;
} tia_input_data_t;

typedef struct tia_s {
  uint8_t dot;
  uint16_t scanline;
  bool rdy;
  bool vsync;
  bool vsync_done;
  bool vblank;
  bool hmove_executed;
  uint16_t wsync_count; /* For debugging. */
  tia_input_data_t input[TIA_INPUTS];
  tia_object_data_t object[TIA_OBJECTS];
  uint8_t playfield[3];
  bool playfield_reflect;
  bool playfield_score_mode;
  bool playfield_priority;
  uint8_t playfield_color;
  uint8_t background_color;
  bool collision[TIA_COLLISIONS];
  uint8_t scanline_colors[TIA_SCANLINE_WIDTH];
  tia_object_t scanline_object[TIA_SCANLINE_WIDTH];
} tia_t;

#define TIA_VSYNC   0x00 /* Vertical Sync Set-clear */
#define TIA_VBLANK  0x01 /* Vertical Blank Set-clear */
#define TIA_WSYNC   0x02 /* Wait for Leading Edge of Horizontal Blank */
#define TIA_RSYNC   0x03 /* Reset Horizontal Sync Counter */
#define TIA_NUSIZ0  0x04 /* Number & Size Player and Missile 0 */
#define TIA_NUSIZ1  0x05 /* Number & Size Player and Missile 1 */
#define TIA_COLUP0  0x06 /* Color & Luminance Player 0 and Missile 0 */
#define TIA_COLUP1  0x07 /* Color & Luminance Player 1 and Missile 1 */
#define TIA_COLUPF  0x08 /* Color & Luminance Playfield and Ball */
#define TIA_COLUBK  0x09 /* Color & Luminance Background */
#define TIA_CTRLPF  0x0A /* Control Playfield Ball Size & Collisions */
#define TIA_REFP0   0x0B /* Reflect Player 0 */
#define TIA_REFP1   0x0C /* Reflect Player 1 */
#define TIA_PF0     0x0D /* Playfield Register Byte 0 */
#define TIA_PF1     0x0E /* Playfield Register Byte 1 */
#define TIA_PF2     0x0F /* Playfield Register Byte 2 */
#define TIA_RESP0   0x10 /* Reset Player 0 */
#define TIA_RESP1   0x11 /* Reset Player 1 */
#define TIA_RESM0   0x12 /* Reset Missile 0 */
#define TIA_RESM1   0x13 /* Reset Missile 1 */
#define TIA_RESBL   0x14 /* Reset Ball */
#define TIA_AUDC0   0x15 /* Audio Control 0 */
#define TIA_AUDC1   0x16 /* Audio Control 1 */
#define TIA_AUDF0   0x17 /* Audio Frequency 0 */
#define TIA_AUDF1   0x18 /* Audio Frequency 1 */
#define TIA_AUDV0   0x19 /* Audio Volume 0 */
#define TIA_AUDV1   0x1A /* Audio Volume 1 */
#define TIA_GRP0    0x1B /* Graphics Enable Player 0 */
#define TIA_GRP1    0x1C /* Graphics Enable Player 1 */
#define TIA_ENAM0   0x1D /* Graphics Enable Missile 0 */
#define TIA_ENAM1   0x1E /* Graphics Enable Missile 1 */
#define TIA_ENABL   0x1F /* Graphics Enable Ball */
#define TIA_HMP0    0x20 /* Horizontal Motion Player 0 */
#define TIA_HMP1    0x21 /* Horizontal Motion Player 1 */
#define TIA_HMM0    0x22 /* Horizontal Motion Missile 0 */
#define TIA_HMM1    0x23 /* Horizontal Motion Missile 1 */
#define TIA_HMBL    0x24 /* Horizontal Motion Ball */
#define TIA_VDELP0  0x25 /* Vertical Delay Player 0 */
#define TIA_VDELP1  0x26 /* Vertical Delay Player 1 */
#define TIA_VDELBL  0x27 /* Vertical Delay Ball */
#define TIA_RESMP0  0x28 /* Reset Missile 0 to Player 0 */
#define TIA_RESMP1  0x29 /* Reset Missile 1 to Player 1 */
#define TIA_HMOVE   0x2A /* Apply Horizontal Motion */
#define TIA_HMCLR   0x2B /* Clear Horizontal Motion Registers */
#define TIA_CXCLR   0x2C /* Clear Collision Latches */

#define TIA_CXM0P   0x00 /* Collision M0-P1, M0-P0 */
#define TIA_CXM1P   0x01 /* Collision M1-P0, M1-P1 */
#define TIA_CXP0FB  0x02 /* Collision P0-PF, P0-BL */
#define TIA_CXP1FB  0x03 /* Collision P1-PF, P1-BL */
#define TIA_CXM0FB  0x04 /* Collision M0-PF, M0-BL */
#define TIA_CXM1FB  0x05 /* Collision M1-PF, M1-BL */
#define TIA_CXBLPF  0x06 /* Collision BL-PF */
#define TIA_CXPPMM  0x07 /* Collision P0-P1, M0-M1 */
#define TIA_INPT0   0x08 /* Input Pot Port 0 */
#define TIA_INPT1   0x09 /* Input Pot Port 1 */
#define TIA_INPT2   0x0A /* Input Pot Port 2 */
#define TIA_INPT3   0x0B /* Input Pot Port 3 */
#define TIA_INPT4   0x0C /* Input Joystick Button P0 */
#define TIA_INPT5   0x0D /* Input Joystick Button P1 */

void tia_init(tia_t *tia, mem_t *mem);
void tia_execute(tia_t *tia);
void tia_dump(FILE *fh, tia_t *tia);

#endif /* _TIA_H */
