#include <stdint.h>

static volatile __data __at (0x78) uint8_t irq_counter_78;

void increment_irq_counter_78(void) __interrupt
{
    ++irq_counter_78;
}
