#ifndef _MOS6507_H
#define _MOS6507_H

#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

typedef struct mos6507_status_s {
  uint8_t n : 1; /* Negative */
  uint8_t v : 1; /* Overflow */
  uint8_t b : 1; /* Break */
  uint8_t d : 1; /* Decimal */
  uint8_t i : 1; /* Interrupt Disable */
  uint8_t z : 1; /* Zero */
  uint8_t c : 1; /* Carry */
} mos6507_status_t;

typedef struct mos6507_s {
  uint16_t pc;         /* Program Counter */
  uint8_t a;           /* Accumulator */
  uint8_t x;           /* X Register */
  uint8_t y;           /* Y Register */
  uint8_t sp;          /* Stack Pointer */
  mos6507_status_t sr; /* Status Register */
  uint8_t cycles;      /* Internal Cycle Counter */
} mos6507_t;

#define MOS6507_VECTOR_RESET_LOW  0xFFFC
#define MOS6507_VECTOR_RESET_HIGH 0xFFFD
#define MOS6507_VECTOR_IRQ_LOW    0xFFFE
#define MOS6507_VECTOR_IRQ_HIGH   0xFFFF

void mos6507_execute(mos6507_t *cpu, mem_t *mem);
void mos6507_reset(mos6507_t *cpu, mem_t *mem);

#endif /* _MOS6507_H */
