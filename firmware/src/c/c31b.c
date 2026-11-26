#include "stock_registers.h"

static __sbit __at (0x29) service_mode_29;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) service_link_ready_6b;
static __sbit __at (0x6c) shutdown_option_6c;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0xe5) ACC_BIT5;
static __sbit __at (0xe6) ACC_BIT6;
static __sbit __at (0xe7) ACC_BIT7;

static volatile __xdata __at (0xf47d) uint8_t shutdown_options;
static volatile __xdata __at (0xf4b6) uint8_t service_probe_count;

void capture_service_probe_options(void)
{
    ACC = (uint8_t)(service_probe_count - 1) & 7;
    ACC |= 0xf0;
    ACC_BIT7 = !extra_state_enabled;
    ACC_BIT6 = !shutdown_option_6c;
    ACC_BIT5 = !service_link_ready_6b;
    ACC_BIT4 = !service_mode_29;
    shutdown_options = ACC;
}
