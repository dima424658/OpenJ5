#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x54) startup_flag_54;

static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __xdata __at (0xf824) uint8_t last_throttle_sample;
static volatile __xdata __at (0xf827) uint8_t throttle_zero_reference;
static volatile __xdata __at (0xf8b2) uint8_t throttle_position_baseline;

extern volatile __code const uint8_t throttle_transfer_gain;
extern volatile __code const uint8_t throttle_max_position;
extern volatile __code const uint8_t startup_throttle_zero;
extern __code const uint8_t throttle_change_limit;
extern __code const uint8_t economy_throttle_boundary[32];

extern void read_adc_8bit(void);
extern void abs_accumulator_i8(void);
extern void evaluate_air_charge_inputs(void);
void filter_throttle_position(void);
void use_throttle_fault_fallback(void);

void continue_sensor_diagnostics(void)
{
    CY = throttle_adc_low | throttle_adc_high;
    if (CY) {
        use_throttle_fault_fallback();
        return;
    }

    B = 3;
    read_adc_8bit();
    B = ACC;
    ACC = throttle_zero_reference;
    const uint8_t sample = B;
    B = ACC;
    ACC = sample;
    if (ACC < B) {
        stock_r0 = 0;
    } else {
        B = (uint8_t)(ACC - B);
        const uint16_t travel = (uint16_t)B * throttle_transfer_gain;
        stock_r0 = (uint8_t)(travel >> 8);
        ACC = throttle_max_position;
        if (stock_r0 > ACC)
            stock_r0 = ACC;
    }

    filter_throttle_position();
}

void filter_throttle_position(void)
{
    B = throttle_change_limit;
    ACC = last_throttle_sample - stock_r0;
    abs_accumulator_i8();
    if (ACC < B)
        throttle_position = stock_r0;
    last_throttle_sample = stock_r0;
    evaluate_air_charge_inputs();
}

void use_throttle_fault_fallback(void)
{
    if (startup_flag_54) {
        throttle_position = 0;
    } else {
        ACC = economy_throttle_boundary[rpm_nearest_half_bin];
        last_throttle_sample = ACC;
        throttle_position = ACC;
    }

    ACC = startup_throttle_zero;
    throttle_zero_reference = ACC;
    throttle_position_baseline = ACC;
    evaluate_air_charge_inputs();
}
