#include <stdint.h>

static __sbit __at (0x4e) shutdown_flag_4e;
static __sbit __at (0x4d) shutdown_flag_4d;
static __sbit __at (0x62) idle_transition_hold_62;
static __sbit __at (0xfe) output_control_fe;
static __sbit __at (0x31) event_flag_31;
static __sbit __at (0xbd) event_irq_high_priority;

static volatile __xdata __at (0xf810) uint8_t status_f810;
static volatile __xdata __at (0xf80e) uint8_t state_f80e;
static volatile __xdata __at (0xf95b) uint8_t state_f95b;
static volatile __xdata __at (0xf95c) uint8_t state_f95c;
static volatile __xdata __at (0xf999) uint8_t relay_outputs;

void reset_after_supply_return(void)
{
    shutdown_flag_4e = 0;
    shutdown_flag_4d = 0;
    idle_transition_hold_62 = 1;
    status_f810 &= 0xfc;
    output_control_fe = 1;
    state_f80e = 0;
    state_f95b = 0;
    state_f95c = 0;
    relay_outputs |= 0x60;
    event_flag_31 = 1;
    event_irq_high_priority = 1;
}
