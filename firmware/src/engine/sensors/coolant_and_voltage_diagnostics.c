#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x18) system_voltage_low;
static __sbit __at (0x20) system_voltage_high;
static __sbit __at (0x4c) ignition_supply_low;
static volatile __data __at (0x4b) uint8_t ignition_voltage_raw;
static volatile __data __at (0x50) uint8_t injector_voltage_offset;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __xdata __at (0xf80d) uint8_t ignition_supply_raw;
static volatile __xdata __at (0xf81b) uint16_t ignition_supply_sum;
static volatile __xdata __at (0xf8b0) uint8_t sensor_state_f8b0;
static volatile __xdata __at (0xf931) uint8_t knock_filter_frequency;
static volatile __xdata __at (0xf93b) uint8_t knock_filter_point_raw;
static volatile __xdata __at (0xf978) uint8_t voltage_quantized;
static volatile __xdata __at (0xf9b1) uint8_t voltage_fault_count;

extern __code const uint8_t knock_filter_point_by_rpm[32];
extern __code const uint8_t knock_filter_frequencies[64];
extern __code const uint8_t injector_dynamic_offset_raw[16];
extern volatile __code const uint8_t system_voltage_min_raw;
extern volatile __code const uint8_t system_voltage_max_raw;
extern volatile __code const uint8_t system_voltage_offset_raw;
extern volatile __code const uint8_t system_voltage_gain_q7;
extern volatile __code const uint8_t system_voltage_fallback_rpm;
extern volatile __code const uint8_t system_voltage_fallback_raw;
extern __code const uint8_t system_voltage_fallback_high;
extern __code const uint8_t system_voltage_fault_delay;

extern void shift_left_saturating_3(void);
extern void interp_descending_curve_q4(void);
extern void update_throttle_zero_samples(void);

void evaluate_system_voltage(void);
void count_low_system_voltage(void);
void count_high_system_voltage(void);
void select_system_voltage_fallback(void);
void update_running_injector_offset(void);

void coolant_update_continued(void)
{
    sensor_state_f8b0 = 3;
    ACC = knock_filter_point_by_rpm[rpm_nearest_half_bin];
    knock_filter_point_raw = ACC;
    stock_r1 = ACC & 0x3f;
    knock_filter_frequency = knock_filter_frequencies[stock_r1];
    evaluate_system_voltage();
}

void evaluate_system_voltage(void)
{
    stock_r01 = ignition_supply_sum;
    shift_left_saturating_3();
    ignition_supply_raw = stock_r1;

    B = system_voltage_min_raw;
    if (stock_r1 < B) {
        count_low_system_voltage();
        return;
    }
    B = system_voltage_max_raw;
    if (stock_r1 >= B) {
        count_high_system_voltage();
        return;
    }

    system_voltage_low = 0;
    system_voltage_high = 0;
    voltage_fault_count = 0;
    ignition_voltage_raw = stock_r1;
    update_running_injector_offset();
}

void count_low_system_voltage(void)
{
    if (!ignition_supply_low && !system_voltage_low) {
        B = system_voltage_fault_delay;
        ACC = voltage_fault_count + 1;
        voltage_fault_count = ACC;
        if (ACC >= B) {
            system_voltage_low = 1;
            system_voltage_high = 0;
            voltage_fault_count = 0;
        }
    }
    select_system_voltage_fallback();
}

void count_high_system_voltage(void)
{
    if (!system_voltage_high) {
        B = system_voltage_fault_delay;
        ACC = voltage_fault_count + 1;
        voltage_fault_count = ACC;
        if (ACC >= B) {
            system_voltage_low = 0;
            system_voltage_high = 1;
            voltage_fault_count = 0;
        }
    }
    select_system_voltage_fallback();
}

void select_system_voltage_fallback(void)
{
    B = system_voltage_fallback_rpm;
    if (rpm_eighth < B)
        ignition_voltage_raw = system_voltage_fallback_raw;
    else
        ignition_voltage_raw = system_voltage_fallback_high;
    update_running_injector_offset();
}

void update_running_injector_offset(void)
{
    B = system_voltage_offset_raw;
    ACC = ignition_voltage_raw;
    if (ACC < B)
        ACC = 0;
    else
        ACC -= B;
    B = ACC;
    uint16_t scaled = (uint16_t)B * system_voltage_gain_q7;
    ACC = scaled >= 0x8000 ? UINT8_MAX : (uint8_t)(scaled >> 7);
    voltage_quantized = ACC;
    STOCK_DPTR = (uint16_t)injector_dynamic_offset_raw;
    interp_descending_curve_q4();
    injector_voltage_offset = ACC;
    update_throttle_zero_samples();
}
