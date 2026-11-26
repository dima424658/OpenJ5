#include "stock_registers.h"

static __sfr __at (0xf9) knock_serial_port;
static __sbit __at (0xf0) command_bit_0;
static __sbit __at (0xf1) command_bit_1;
static __sbit __at (0xf2) command_bit_2;
static __sbit __at (0xf3) command_bit_3;
static __sbit __at (0xf4) command_bit_4;
static __sbit __at (0xf5) command_bit_5;
static __sbit __at (0xf6) command_bit_6;
static __sbit __at (0xf7) command_bit_7;

/* F9 is a byte-addressed SFR. Transfer its data bit through ACC. */
#define CLOCK_KNOCK_BIT(bit)         \
    do {                             \
        knock_serial_port &= 0xef;   \
        ACC = knock_serial_port;     \
        ACC_SIGN = bit;              \
        knock_serial_port = ACC;     \
        knock_serial_port |= 0x10;   \
        __asm nop __endasm;          \
    } while (0)

void send_knock_command(void)
{
    CLOCK_KNOCK_BIT(command_bit_7);
    CLOCK_KNOCK_BIT(command_bit_6);
    CLOCK_KNOCK_BIT(command_bit_5);
    CLOCK_KNOCK_BIT(command_bit_4);
    CLOCK_KNOCK_BIT(command_bit_3);
    CLOCK_KNOCK_BIT(command_bit_2);
    CLOCK_KNOCK_BIT(command_bit_1);
    CLOCK_KNOCK_BIT(command_bit_0);
}

/* The original settling interval is straight-line timing. */
#define KNOCK_DELAY_7()              \
    __asm                            \
        nop                          \
        nop                          \
        nop                          \
        nop                          \
        nop                          \
        nop                          \
        nop                          \
    __endasm

void wait_knock_filter(void)
{
    KNOCK_DELAY_7();
    KNOCK_DELAY_7();
    KNOCK_DELAY_7();
    KNOCK_DELAY_7();
    KNOCK_DELAY_7();
    KNOCK_DELAY_7();
    KNOCK_DELAY_7();
}
