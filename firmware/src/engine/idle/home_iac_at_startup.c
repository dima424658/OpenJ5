#include "stock_registers.h"

enum { IAC_HOMING_POSITION = 127 };

static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __xdata __at (0xf80a) uint8_t startup_coolant_temperature_raw;

extern volatile __code const uint8_t cold_start_coolant_threshold;
extern volatile __code const uint8_t cold_start_iac_max_position;
extern volatile __code const uint8_t iac_max_position;

void home_iac_at_startup(void)
{
    startup_coolant_temperature_raw = coolant_temperature_raw;
    iac_position = IAC_HOMING_POSITION;
    iac_target = 0;
    interrupts_enabled = 1;

    /* The interrupt counts steps towards the seat. */
    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        ACC = iac_target;
    } while (ACC != iac_position);

    if (cold_start_coolant_threshold >= coolant_temperature_raw) {
        ACC = cold_start_iac_max_position;
    } else {
        ACC = iac_max_position;
    }
    iac_target = ACC;

    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        ACC = iac_target;
    } while (ACC != iac_position);
}
