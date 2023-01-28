#ifndef _PIA_H
#define _PIA_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"

#define PIA_RAM_SIZE 0x100

typedef struct pia_s {
  uint8_t ram[PIA_RAM_SIZE];

  uint8_t port_a;
  uint8_t port_b;
  uint8_t port_a_ddr;
  uint8_t port_b_ddr;

  uint8_t timer;
  uint16_t interval;
  uint16_t cycle;
  bool underflow;
} pia_t;

#define PIA_SWCHA        0x280 /* Port A */
#define PIA_SWACNT       0x281 /* Port A DDR */
#define PIA_SWCHB        0x282 /* Port B */
#define PIA_SWBCNT       0x283 /* Port B DDR */
#define PIA_INTIM        0x284 /* Timer Output */
#define PIA_INTIM_COPY   0x286 /* Timer Output (Copy) */
#define PIA_INSTAT       0x285 /* Timer Status */
#define PIA_INSTAT_COPY  0x287 /* Timer Status (Copy) */
#define PIA_TIM1T        0x284 /* Timer 1 Clock Interval */
#define PIA_TIM8T        0x285 /* Timer 8 Clock Interval */
#define PIA_TIM64T       0x286 /* Timer 64 Clock Interval */
#define PIA_T1024T       0x287 /* Timer 1024 Clock Interval */

void pia_init(pia_t *pia, mem_t *mem);
void pia_execute(pia_t *pia);
void pia_dump(FILE *fh, pia_t *pia);

#endif /* _PIA_H */
