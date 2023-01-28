#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "mem.h"
#include "main.h"



void mem_init(mem_t *mem)
{
  mem->tia_read   = NULL;
  mem->tia_write  = NULL;
  mem->pia_read   = NULL;
  mem->pia_write  = NULL;
  mem->cart_read  = NULL;
  mem->cart_write = NULL;
  mem->tia  = NULL;
  mem->pia  = NULL;
  mem->cart = NULL;
}



uint8_t mem_read(mem_t *mem, uint16_t address)
{
  address &= 0x1FFF; /* Mirroring */

  if ((address & 0x1000) > 0) { /* A12 = 1, Cartridge */
    if (mem->cart_read != NULL && mem->cart != NULL) {
      return (mem->cart_read)(mem->cart, address);
    } else {
      panic("Cartridge read hook not installed! Address: 0x%04x\n", address);
    }

  } else { /* A12 = 0 */
    if ((address & 0x80) > 0) { /* A7 = 1, PIA */
      if (mem->pia_read != NULL && mem->pia != NULL) {
        return (mem->pia_read)(mem->pia, address);
      } else {
        panic("PIA read hook not installed! Address: 0x%04x\n", address);
      }

    } else { /* A7 = 0, TIA */
      if (mem->tia_read != NULL && mem->tia != NULL) {
        return (mem->tia_read)(mem->tia, address);
      } else {
        panic("TIA read hook not installed! Address: 0x%04x\n", address);
      }
    }
  }

  return 0;
}



void mem_write(mem_t *mem, uint16_t address, uint8_t value)
{
  address &= 0x1FFF; /* Mirroring */

  if ((address & 0x1000) > 0) { /* A12 = 1, Cartridge */
    if (mem->cart_write != NULL && mem->cart != NULL) {
      (mem->cart_write)(mem->cart, address, value);
    } else {
      panic("Cartridge write hook not installed! Address: 0x%04x\n", address);
    }

  } else { /* A12 = 0 */
    if ((address & 0x80) > 0) { /* A7 = 1, PIA */
      if (mem->pia_write != NULL && mem->pia != NULL) {
        (mem->pia_write)(mem->pia, address, value);
      } else {
        panic("PIA write hook not installed! Address: 0x%04x\n", address);
      }

    } else { /* A7 = 0, TIA */
      if (mem->tia_write != NULL && mem->tia != NULL) {
        (mem->tia_write)(mem->tia, address, value);
      } else {
        panic("TIA write hook not installed! Address: 0x%04x\n", address);
      }
    }
  }
}



static void mem_dump_16(FILE *fh, mem_t *mem, uint16_t start, uint16_t end)
{
  int i;
  uint16_t address;

  fprintf(fh, "$%04x   ", start & 0xFFF0);

  /* Hex */
  for (i = 0; i < 16; i++) {
    address = (start & 0xFFF0) + i;
    if ((address >= start) && (address <= end)) {
      fprintf(fh, "%02x ", mem_read(mem, address));
    } else {
      fprintf(fh, "   ");
    }
    if (i % 4 == 3) {
      fprintf(fh, " ");
    }
  }

  /* Character */
  for (i = 0; i < 16; i++) {
    address = (start & 0xFFF0) + i;
    if ((address >= start) && (address <= end)) {
      if (isprint(mem_read(mem, address))) {
        fprintf(fh, "%c", mem_read(mem, address));
      } else {
        fprintf(fh, ".");
      }
    } else {
      fprintf(fh, " ");
    }
  }

  fprintf(fh, "\n");
}



void mem_dump(FILE *fh, mem_t *mem, uint16_t start, uint16_t end)
{
  int i;
  mem_dump_16(fh, mem, start, end);
  for (i = (start & 0xFFF0) + 16; i <= end; i += 16) {
    mem_dump_16(fh, mem, i, end);
  }
}



