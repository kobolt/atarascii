#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef uint8_t (*mem_read_hook_t)(void *, uint16_t);
typedef void (*mem_write_hook_t)(void *, uint16_t, uint8_t);

typedef struct mem_s {
  mem_read_hook_t  tia_read;
  mem_write_hook_t tia_write;
  mem_read_hook_t  pia_read;
  mem_write_hook_t pia_write;
  mem_read_hook_t  cart_read;
  mem_write_hook_t cart_write;
  void *tia;
  void *pia;
  void *cart;
} mem_t;

#define MEM_PAGE_STACK 0x100

void mem_init(mem_t *mem);
uint8_t mem_read(mem_t *mem, uint16_t address);
void mem_write(mem_t *mem, uint16_t address, uint8_t value);
void mem_dump(FILE *fh, mem_t *mem, uint16_t start, uint16_t end);

#endif /* _MEM_H */
