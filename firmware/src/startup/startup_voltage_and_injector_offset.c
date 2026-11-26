#include "stock_registers.h"

enum {
    THROTTLE_ADC_CHANNEL = 3,
    IGNITION_VOLTAGE_ADC_CHANNEL = 1,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __data __at (0x4b) uint8_t ignition_voltage_raw;
static volatile __data __at (0x50) uint8_t injector_voltage_offset;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __xdata __at (0xf80d) uint8_t ignition_supply_raw;
static volatile __xdata __at (0xf827) uint8_t throttle_zero_reference;
static volatile __xdata __at (0xf8b0) uint8_t startup_state_f8b0;
static volatile __xdata __at (0xf8b8) uint8_t previous_throttle_position;
static volatile __xdata __at (0xf978) uint8_t voltage_quantized;
static volatile __xdata __at (0xf98c) uint8_t startup_throttle_position;

extern volatile __code const uint8_t throttle_transfer_gain;
extern volatile __code const uint8_t throttle_max_position;
extern volatile __code const uint8_t system_voltage_min_raw;
extern volatile __code const uint8_t system_voltage_max_raw;
extern volatile __code const uint8_t system_voltage_offset_raw;
extern volatile __code const uint8_t system_voltage_gain_q7;
extern volatile __code const uint8_t system_voltage_fallback_raw;
extern __code const uint8_t injector_dynamic_offset_raw[16];

extern void read_adc_8bit(void);
extern void interp_descending_curve_q4(void);
extern void continue_startup_sensor_setup(void);
void sample_startup_voltage(void);
void init_injector_voltage_offset(void);

void warmup_setup_continue(void)
{
    startup_state_f8b0 = 3;
    B = THROTTLE_ADC_CHANNEL;
    read_adc_8bit();
    B = throttle_zero_reference;
    if (ACC < B) {
        stock_r0 = 0;
    } else {
        B = (uint8_t)(ACC - B);
        uint16_t travel = (uint16_t)B * throttle_transfer_gain;
        stock_r0 = (uint8_t)(travel >> 8);
        ACC = throttle_max_position;
        if (stock_r0 > ACC) {
            stock_r0 = ACC;
        }
    }

    ACC = stock_r0;
    throttle_position = ACC;
    startup_throttle_position = ACC;
    previous_throttle_position = ACC;
    sample_startup_voltage();
}

void sample_startup_voltage(void)
{
    B = IGNITION_VOLTAGE_ADC_CHANNEL;
    read_adc_8bit();
    ignition_supply_raw = ACC;
    stock_r1 = ACC;
    B = system_voltage_min_raw;
    if (stock_r1 < B) {
        goto use_fallback;
    }
    B = system_voltage_max_raw;
    if (stock_r1 >= B) {
        goto use_fallback;
    }
    ignition_voltage_raw = stock_r1;
    init_injector_voltage_offset();
    return;

use_fallback:
    ignition_voltage_raw = system_voltage_fallback_raw;
    init_injector_voltage_offset();
}

void init_injector_voltage_offset(void)
{
    B = system_voltage_offset_raw;
    ACC = ignition_voltage_raw;
    if (ACC < B) {
        ACC = 0;
    } else {
        ACC -= B;
    }
    B = ACC;
    uint16_t scaled = (uint16_t)B * system_voltage_gain_q7;
    ACC = scaled >= 0x8000 ? UINT8_MAX : (uint8_t)(scaled >> 7);
    voltage_quantized = ACC;
    STOCK_DPTR = (uint16_t)injector_dynamic_offset_raw;
    interp_descending_curve_q4();
    injector_voltage_offset = ACC;
    continue_startup_sensor_setup();
}
