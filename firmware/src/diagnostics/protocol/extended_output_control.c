#include "stock_registers.h"

static __sbit __at (0x7f) serial_extended_length;
static volatile __data __at (0x7a) uint8_t idle_control_override;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __xdata __at (0xf849) uint8_t idle_rpm_target;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

void apply_output_control_41(void)
{
    switch (stock_r3) {
    case 0:
        idle_control_override &= 0xdf;
        break;
    case 1:
        break;
    case 7:
        idle_control_override |= 0x20;
        iac_target = serial_rx_buffer[6 + (serial_extended_length ? 1 : 0)];
        break;
    default:
        stock_r2 = 0x12;
        return;
    }
    stock_r2 = 0;
}

void apply_output_control_42(void)
{
    switch (stock_r3) {
    case 0:
        idle_control_override &= 0xbf;
        break;
    case 1:
        break;
    case 7:
        idle_control_override |= 0x40;
        idle_rpm_target = serial_rx_buffer[6 + (serial_extended_length ? 1 : 0)];
        break;
    default:
        stock_r2 = 0x12;
        return;
    }
    stock_r2 = 0;
}
