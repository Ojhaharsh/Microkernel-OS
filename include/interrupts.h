#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

void interrupts_init(void);
void pit_init(uint32_t hz);

#endif // INTERRUPTS_H
