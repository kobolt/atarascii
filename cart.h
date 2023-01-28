#ifndef _CART_H
#define _CART_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"

#define CART_BANK_SIZE 0x1000
#define CART_BANK_MAX 2

typedef enum cart_type_s {
  CART_TYPE_NONE,
  CART_TYPE_2K,
  CART_TYPE_4K,
  CART_TYPE_8K,
} cart_type_t;

typedef struct cart_s {
  cart_type_t type;
  uint8_t bank[CART_BANK_MAX][CART_BANK_SIZE];
  int bank_select;
} cart_t;

void cart_init(cart_t *cart, mem_t *mem);
int cart_load(cart_t *cart, const char *filename);
void cart_dump(FILE *fh, cart_t *cart);

#endif /* _CART_H */
