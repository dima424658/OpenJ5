#include "stock_registers.h"

static __sbit __at (0x7f) serial_extended_length;
static volatile __data __at (0x7b) uint8_t purge_override_flags;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];
static volatile __xdata __at (0xf806) uint8_t purge_duty;

void apply_output_control_0d(void)
{
    switch (stock_r3) {
    case 0:
        purge_override_flags &= 0xfb;
        /* Return the existing command after releasing the override. */
        stock_r2 = purge_duty != 0;
        break;
    case 1:
        stock_r2 = purge_duty != 0;
        break;
    case 6:
        purge_override_flags |= 4;
        if (serial_rx_buffer[6 + (serial_extended_length ? 1 : 0)]) {
            purge_duty = 0x40;
            stock_r2 = 1;
        } else {
            purge_duty = 0;
            stock_r2 = 0;
        }
        break;
    default:
        stock_r2 = 0x12;
        break;
    }
}
