#include <stdint.h>

static __sfr __at (0xf9) serial_port;
static __sbit __at (0xec) serial_clock;

void pulse_startup_serial_clock(void)
{
    serial_port &= 0xfd;
    __asm nop __endasm;
    serial_clock = 1;
    serial_clock = 0;
    __asm nop __endasm;
    serial_port |= 0x02;
}
