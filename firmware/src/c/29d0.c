#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr __at (0xf9) startup_io_port;
static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xae) iac_tick_enabled;
static __sbit __at (0xbd) event_irq_high_priority;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0x48) timer_event_48;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x5d) fuel_mode_flag_5d;
static __sbit __at (0x61) previous_idle_regulating;
static __sbit __at (0x63) previous_fuel_mode_63;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe1) ACC_BIT1;

static volatile __data __at (0x20) uint8_t configuration_flags_1;
static volatile __data __at (0x21) uint8_t configuration_flags_2;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __data __at (0x7b) uint8_t knock_window_override;
static volatile __xdata __at (0xf87f) uint8_t startup_ignition_flags;
static volatile __xdata __at (0xf946) uint8_t knock_window_start;
static volatile __xdata __at (0xf947) uint8_t knock_window_end;

extern volatile __code const uint8_t cold_ignition_resume_temp;
extern volatile __code const uint8_t cold_ignition_disable_temp;
extern volatile __code const uint8_t ecu_configuration_flags_1;
extern volatile __code const uint8_t ecu_configuration_flags_2;
extern __code const uint8_t knock_window_start_by_rpm[32];
extern __code const uint8_t knock_window_end_by_rpm[32];

extern void pulse_startup_serial_clock(void);
extern void apply_knock_window_phases(void);

void wait_for_control_tick(void);
void update_startup_ign_flags(void);
void load_cycle_configuration(void);

void continue_startup_after_iac(void)
{
    pulse_startup_serial_clock();
    event_irq_high_priority = 1;
    interrupts_enabled = 1;
    wait_for_control_tick();
}

void wait_for_control_tick(void)
{
    while (!timer_event_48) {
    }
    timer_event_48 = 0;
    iac_tick_enabled = 1;
    iac_tick_high_priority = 1;
    update_startup_ign_flags();
}

#define SET_COLD_IGNITION_FLAG(threshold) \
    do {                                  \
        B = threshold;                    \
        CY = coolant_temperature_raw < B; \
        ACC = startup_ignition_flags;     \
        ACC_BIT1 = CY;                    \
        startup_ignition_flags = ACC;     \
    } while (0)

void update_startup_ign_flags(void)
{
    ACC = startup_ignition_flags;
    if (!ACC_BIT1) {
        SET_COLD_IGNITION_FLAG(cold_ignition_resume_temp);
    } else {
        SET_COLD_IGNITION_FLAG(cold_ignition_disable_temp);
    }
    load_cycle_configuration();
}

void load_cycle_configuration(void)
{
    startup_io_port &= 0xdf;
    previous_idle_regulating = idle_regulating;
    previous_fuel_mode_63 = fuel_mode_flag_5d;
    configuration_flags_1 = ecu_configuration_flags_1 | 0x01;
    configuration_flags_2 = ecu_configuration_flags_2;

    if (!(knock_window_override & 0x20)) {
        knock_window_start = knock_window_start_by_rpm[rpm_nearest_half_bin];
    }
    if (!(knock_window_override & 0x40)) {
        knock_window_end = knock_window_end_by_rpm[rpm_nearest_half_bin];
    }
    apply_knock_window_phases();
}
