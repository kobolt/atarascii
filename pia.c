#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pia.h"
#include "mem.h"
#include "gui.h"
#include "console.h"
#include "tas.h"
#include "main.h"



static uint8_t pia_read_hook(void *pia, uint16_t address)
{
  if ((address & 0x200) > 0) { /* I/O */
    address &= 0x287; /* Mirroring */

    switch (address) {
    case PIA_SWCHA:
      return ((pia_t *)pia)->port_a;

    case PIA_SWACNT:
      return ((pia_t *)pia)->port_a_ddr;

    case PIA_SWCHB:
      return ((pia_t *)pia)->port_b;

    case PIA_SWBCNT:
      return ((pia_t *)pia)->port_b_ddr;

    case PIA_INTIM:
    case PIA_INTIM_COPY:
      ((pia_t *)pia)->underflow = false; /* Reset on read! */
      return ((pia_t *)pia)->timer;

    case PIA_INSTAT:
    case PIA_INSTAT_COPY:
      /* Not implemented. */
      return 0;

    default:
      panic("PIA I/O read on unhandled address: 0x%04x\n", address);
      return 0;
    }

  } else { /* RAM */
    return ((pia_t *)pia)->ram[address & 0x7F];
  }
}



static void pia_write_hook(void *pia, uint16_t address, uint8_t value)
{
  if ((address & 0x200) > 0) { /* I/O */
    address &= 0x287; /* Mirroring */

    switch (address) {
    case PIA_SWCHA:
      ((pia_t *)pia)->port_a = value;
      break;

    case PIA_SWACNT:
      ((pia_t *)pia)->port_a_ddr = value;
      break;

    case PIA_SWCHB:
      ((pia_t *)pia)->port_b = value;
      break;

    case PIA_SWBCNT:
      ((pia_t *)pia)->port_b_ddr = value;
      break;

    case PIA_TIM1T:
      ((pia_t *)pia)->interval = 1;
      ((pia_t *)pia)->timer = value - 1;
      ((pia_t *)pia)->cycle = 0;
      ((pia_t *)pia)->underflow = false;
      break;

    case PIA_TIM8T:
      ((pia_t *)pia)->interval = 8;
      ((pia_t *)pia)->timer = value - 1;
      ((pia_t *)pia)->cycle = 0;
      ((pia_t *)pia)->underflow = false;
      break;

    case PIA_TIM64T:
      ((pia_t *)pia)->interval = 64;
      ((pia_t *)pia)->timer = value - 1;
      ((pia_t *)pia)->cycle = 0;
      ((pia_t *)pia)->underflow = false;
      break;

    case PIA_T1024T:
      ((pia_t *)pia)->interval = 1024;
      ((pia_t *)pia)->timer = value - 1;
      ((pia_t *)pia)->cycle = 0;
      ((pia_t *)pia)->underflow = false;
      break;

    default:
      panic("PIA I/O write (0x%02x) on unhandled address: 0x%04x\n",
        value, address);
    }

  } else { /* RAM */
    ((pia_t *)pia)->ram[address & 0x7F] = value;
  }
}



void pia_init(pia_t *pia, mem_t *mem)
{
  int i;

  mem->pia = pia;
  mem->pia_read  = pia_read_hook;
  mem->pia_write = pia_write_hook;

  for (i = 0; i < PIA_RAM_SIZE; i++) {
    pia->ram[i] = 0xFF;
  }

  pia->port_a     = 0xFF; /* No joystick movement. */
  pia->port_b     = 0b0001011; /* Release reset and select buttons. */
  pia->port_a_ddr = 0;
  pia->port_b_ddr = 0;

  pia->timer     = 0;
  pia->interval  = 1024;
  pia->cycle     = 0;
  pia->underflow = false;
}



void pia_execute(pia_t *pia)
{
  uint8_t keep;

  pia->cycle++;
  if (pia->cycle > pia->interval || pia->underflow) {
    pia->cycle = 0;
    pia->timer--;
    if (pia->timer == 0xFF) {
      /* Timer decrements every clock cycle if timer underflows. */
      pia->underflow = true;
    }
  }

  /* Keep the existing value on the bit if it is an output. */
  keep = pia->port_a & pia->port_a_ddr;
  if (tas_is_active()) {
    pia->port_a = tas_get_joystick_movement();
  } else {
    pia->port_a = gui_get_joystick_movement() &
                  console_get_joystick_movement();
  }
  pia->port_a |= keep;

  keep = pia->port_b & pia->port_b_ddr;
  if (tas_is_active()) {
    pia->port_b = tas_get_system_switches();
  } else {
    pia->port_b = gui_get_system_switches() &
                  console_get_system_switches();
  }
  pia->port_b |= keep;
}



static void pia_port_dump(FILE *fh, uint8_t value, uint8_t ddr)
{
  for (int i = 0; i < 8; i++) {
    fprintf(fh, "  %d %c--%c %d\n", i,
      ((ddr >> i) & 1) ? ' ' : '<', /* In */
      ((ddr >> i) & 1) ? '>' : ' ', /* Out */
      (value >> i) & 1);
  }
}



void pia_dump(FILE *fh, pia_t *pia)
{
  fprintf(fh, "Port A:\n");
  pia_port_dump(fh, pia->port_a, pia->port_a_ddr);
  fprintf(fh, "Port B:\n");
  pia_port_dump(fh, pia->port_b, pia->port_b_ddr);
  fprintf(fh, "Timer: %d\n", pia->timer);
  fprintf(fh, "  Interval : %d\n", pia->interval);
  fprintf(fh, "  Cycle    : %d\n", pia->cycle);
  fprintf(fh, "  Underflow: %d\n", pia->underflow);
}



