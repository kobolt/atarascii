#ifndef _MAIN_H
#define _MAIN_H

#include <stdarg.h>

void panic(const char *format, ...);
void debug(void);
void sync(void);

#endif /* _MAIN_H */
