#include "stock_registers.h"

static __sbit __at (0x41) fan_off;
static __sbit __at (0x70) lamp_off_request;
static __sbit __at (0x71) fan_override_on;
static __sbit __at (0x72) ac_override_on;
static __sbit __at (0x73) service_pump_output;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;

#define REQUESTED_OUTPUT() (serial_rx_buffer[6 + (serial_extended_length ? 1 : 0)] != 0)

void apply_output_control_0c(void)
{
    switch (stock_r3) {
    case 0:
        output_override_flags &= 0xfe;
        /* Return the relay state after releasing the override. */
        stock_r2 = (relay_output_latch & 0x10) != 0;
        break;
    case 1:
        stock_r2 = (relay_output_latch & 0x10) != 0;
        break;
    case 6:
        output_override_flags |= 1;
        lamp_off_request = !REQUESTED_OUTPUT();
        stock_r2 = !lamp_off_request;
        break;
    default:
        stock_r2 = 0x12;
        break;
    }
}

void apply_output_control_0b(void)
{
    switch (stock_r3) {
    case 0:
        output_override_flags &= 0xfb;
        stock_r2 = (relay_output_latch & 1) != 0;
        break;
    case 1:
        stock_r2 = (relay_output_latch & 1) != 0;
        break;
    case 6:
        output_override_flags |= 4;
        ac_override_on = REQUESTED_OUTPUT();
        stock_r2 = ac_override_on;
        break;
    default:
        stock_r2 = 0x12;
        break;
    }
}

void apply_output_control_0a(void)
{
    switch (stock_r3) {
    case 0:
        output_override_flags &= 0xfd;
        stock_r2 = !fan_off;
        break;
    case 1:
        stock_r2 = !fan_off;
        break;
    case 6:
        output_override_flags |= 2;
        fan_override_on = REQUESTED_OUTPUT();
        stock_r2 = fan_override_on;
        break;
    default:
        stock_r2 = 0x12;
        break;
    }
}

void apply_output_control_09(void)
{
    switch (stock_r3) {
    case 0:
        output_override_flags &= 0xf7;
        stock_r2 = (relay_output_latch & 0x40) != 0;
        break;
    case 1:
        stock_r2 = (relay_output_latch & 0x40) != 0;
        break;
    case 6:
        output_override_flags |= 8;
        service_pump_output = REQUESTED_OUTPUT();
        stock_r2 = service_pump_output;
        break;
    default:
        stock_r2 = 0x12;
        break;
    }
}
