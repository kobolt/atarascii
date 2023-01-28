#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "cart.h"
#include "mem.h"
#include "main.h"



static char *cart_type(cart_type_t type)
{
  switch (type) {
  case CART_TYPE_NONE:
    return "Uninitialized";
  case CART_TYPE_2K:
    return "2K";
  case CART_TYPE_4K:
    return "4K";
  case CART_TYPE_8K:
    return "8K";
  default:
    return "???";
  }
}



static uint8_t cart_read_hook(void *cart, uint16_t address)
{
  address &= 0xFFF; /* Mirroring */

  if (((cart_t *)cart)->type == CART_TYPE_8K) {
    switch (address) {
    case 0xFF8:
      ((cart_t *)cart)->bank_select = 0;
      break;
    case 0xFF9:
      ((cart_t *)cart)->bank_select = 1;
      break;
    }
  }

  return ((cart_t *)cart)->bank[((cart_t *)cart)->bank_select][address];
}



static void cart_write_hook(void *cart, uint16_t address, uint8_t value)
{
  address &= 0xFFF; /* Mirroring */
  (void)value;

  if (((cart_t *)cart)->type == CART_TYPE_8K) {
    switch (address) {
    case 0xFF8:
      ((cart_t *)cart)->bank_select = 0;
      break;
    case 0xFF9:
      ((cart_t *)cart)->bank_select = 1;
      break;
    }
  }
}



void cart_init(cart_t *cart, mem_t *mem)
{
  cart->type = CART_TYPE_NONE;
  cart->bank_select = 0;
  mem->cart = cart;
  mem->cart_read  = cart_read_hook;
  mem->cart_write = cart_write_hook;
}



int cart_load(cart_t *cart, const char *filename)
{
  FILE *fh;
  int c, n, bank_no;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    return -1;
  }

  cart->type = CART_TYPE_4K; /* Initially, if nothing else triggers. */
  bank_no = 0;
  n = 0;
  while ((c = fgetc(fh)) != EOF) {
    if (bank_no > 0) {
      cart->type = CART_TYPE_8K; /* Simple check, if more than 4K. */
    }
    cart->bank[bank_no][n] = c;
    n++;
    if (n >= CART_BANK_SIZE) {
      n = 0;
      bank_no++;
      if (bank_no >= CART_BANK_MAX) {
        break;
      }
    }
  }

  /* Duplicate to upper area if 2K cartridge: */
  while (bank_no == 0 && n < CART_BANK_SIZE) {
    cart->type = CART_TYPE_2K; /* Simple check, if less than 4K. */
    cart->bank[0][n] = cart->bank[0][n - 0x800];
    n++;
  }

  fclose(fh);
  return 0;
}



void cart_dump(FILE *fh, cart_t *cart)
{
  fprintf(fh, "Cartridge Type: %s\n", cart_type(cart->type));
  fprintf(fh, "Bank Selected: %d\n", cart->bank_select);
}



