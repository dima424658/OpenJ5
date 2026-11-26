#include "stock_registers.h"

enum { HOLD_BASE_IGNITION = 0x04, HOLD_IAC_TARGET = 0x20 };

static __sfr __at (0xf0) B;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x54) poststart_iac_enabled;
static __sbit __at (0x59) idle_regulating;

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __data __at (0x62) uint8_t base_ignition;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x7a) uint8_t ignition_control_7a;

extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t iac_throttle_fault_position;
extern volatile __code const uint8_t fault_ignition_retard_raw;
extern __code const uint8_t startup_exit_rpm_by_coolant[39];
extern __code const uint8_t open_throttle_by_rpm[32];
extern __code const uint8_t closed_throttle_by_rpm[32];
extern __code const int8_t idle_ignition_by_rpm[32];

extern void check_exhaust_dew_point(void);
extern void continue_fuel_mixture_checks(void);

void check_fault_idle_entry(void);
void apply_fault_idle_ignition(void);
void leave_fault_idle(void);

void finish_sensor_fault(void)
{
    if (throttle_adc_low || throttle_adc_high) {
        if (!(ignition_control_7a & HOLD_IAC_TARGET)) {
            iac_target = iac_throttle_fault_position;
        }
    } else {
        if (!(ignition_control_7a & HOLD_IAC_TARGET)) {
            iac_target = iac_max_position;
        }
    }

    if (poststart_iac_enabled) {
        B = startup_exit_rpm_by_coolant[coolant_nearest_index];
        if (engine_rpm_10 >= B) {
            poststart_iac_enabled = 0;
        }
    }
    if (!idle_regulating) {
        check_fault_idle_entry();
        return;
    }
    B = open_throttle_by_rpm[rpm_nearest_half_bin];
    if (throttle_position < B) {
        apply_fault_idle_ignition();
    } else {
        leave_fault_idle();
    }
}

void continue_without_sensor_fault(void)
{
    check_exhaust_dew_point();
}

void check_fault_idle_entry(void)
{
    B = closed_throttle_by_rpm[rpm_nearest_half_bin];
    if (throttle_position < B) {
        apply_fault_idle_ignition();
    } else {
        leave_fault_idle();
    }
}

void apply_fault_idle_ignition(void)
{
    idle_regulating = 1;
    if (!(ignition_control_7a & HOLD_BASE_IGNITION)) {
        B = fault_ignition_retard_raw;
        ACC = idle_ignition_by_rpm[rpm_nearest_half_bin];
        base_ignition = ACC - B;
    }
    continue_fuel_mixture_checks();
}

void leave_fault_idle(void)
{
    idle_regulating = 0;
    continue_fuel_mixture_checks();
}
