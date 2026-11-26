#include "monitor_port9_io.h"
#include "stock_registers.h"

static __sfr __at (0x86) sfr_86;
static __sfr __at (0xf0) B;
static __sfr __at (0xf9) P9;
static __sbit __at (0x97) port1_bit7;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xaf) EA;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x7e) uint8_t monitor_transfer_bytes[2];

extern void poll_monitor_exit_and_io(void);
extern void read_monitor_adc_high(void);

void monitor_handler_e978(void)
{
    do {
        poll_monitor_exit_and_io();
        stock_r1 = 0x1f;
        B = 1;
        read_monitor_adc_high();
        const uint16_t scaled = ((uint16_t)ACC * 0x75u) >> 7;
        ACC = (uint8_t)scaled;
    } while (ACC >= stock_r1);

    EA = 0;
    const uint8_t request = stock_r0;
    stock_r0 = (uint8_t)(uint16_t)monitor_transfer_bytes;
    P9 &= 0xfb;
    ACC = *((volatile __idata uint8_t *)stock_r0);
    ++stock_r0;
    *((volatile __idata uint8_t *)stock_r0) = ACC;
    SHIFT_PORT9_BYTE();
    ++stock_r0;
    P9 |= 4;
    stock_r0 = request;
    sfr_86 = 0x80;
    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    port1_bit7 = 1;

    for (;;) {
    }
}
