#include <stdint.h>

enum {
    DRIVER_CLOCK = 0x01,
    DRIVER_SELECT = 0x04,
};

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sfr __at (0xf9) driver_bus_f9;
static __sbit __at (0xe3) ACC_DATA_OUT;
static __sbit __at (0xe6) ACC_DATA_IN;
static __sbit __at (0xf0) B_BIT0;
static __sbit __at (0xf1) B_BIT1;
static __sbit __at (0xf2) B_BIT2;
static __sbit __at (0xf3) B_BIT3;
static __sbit __at (0xf4) B_BIT4;
static __sbit __at (0xf5) B_BIT5;
static __sbit __at (0xf6) B_BIT6;
static __sbit __at (0xf7) B_BIT7;

static volatile __xdata __at (0xf999) uint8_t relay_output_latch;
static volatile __xdata __at (0xf9ba) uint8_t relay_driver_response;

#define SHIFT_RELAY_BIT(bit) do { \
    driver_bus_f9 |= DRIVER_CLOCK; \
    ACC = driver_bus_f9; \
    ACC_DATA_OUT = (bit); \
    driver_bus_f9 = ACC; \
    (bit) = ACC_DATA_IN; \
    driver_bus_f9 &= ~DRIVER_CLOCK; \
    __asm nop __endasm; \
} while (0)

void update_relay_outputs(void)
{
    driver_bus_f9 &= ~DRIVER_SELECT;
    B = relay_output_latch;

    SHIFT_RELAY_BIT(B_BIT7);
    SHIFT_RELAY_BIT(B_BIT6);
    SHIFT_RELAY_BIT(B_BIT5);
    SHIFT_RELAY_BIT(B_BIT4);
    SHIFT_RELAY_BIT(B_BIT3);
    SHIFT_RELAY_BIT(B_BIT2);
    SHIFT_RELAY_BIT(B_BIT1);
    SHIFT_RELAY_BIT(B_BIT0);

    driver_bus_f9 |= DRIVER_SELECT;
    relay_driver_response = B;
}
