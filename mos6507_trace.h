#ifndef _MOS6507_TRACE_H
#define _MOS6507_TRACE_H

#include <stdio.h>
#include "mos6507.h"
#include "mem.h"

void mos6507_trace_init(void);
void mos6507_trace_add(mos6507_t *cpu, mem_t *mem);
void mos6507_trace_dump(FILE *fh);

#endif /* _MOS6507_TRACE_H */
