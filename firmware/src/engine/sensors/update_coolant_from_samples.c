#include "stock_registers.h"

enum { COOLANT_TRANSFER_OFFSET = 20 };

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x1b) coolant_adc_low;
static __sbit __at (0x23) coolant_adc_high;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __xdata __at (0xf813) uint16_t coolant_sample_sum;
static volatile __xdata __at (0xf873) uint8_t time_since_start_f873;
static volatile __xdata __at (0xf8ac) uint8_t fallback_runtime;
static volatile __xdata __at (0xf9b5) uint8_t startup_coolant_raw;
static volatile __xdata __at (0xf9b6) uint8_t measured_coolant_raw;
static volatile __xdata __at (0xf9b7) uint8_t coolant_rise_latched;

extern __code const uint8_t coolant_adc_limits[2];
extern __code const uint8_t coolant_diagnostic_delay;
extern __code const uint8_t coolant_rise_limits[2];
extern __code const uint8_t coolant_transfer_raw[32];
extern __code const uint8_t coolant_fallback_temperature[16];

extern void shift_left_saturating_3(void);
extern void interp_curve_u8(void);
extern void update_coolant_axis(void);

void convert_measured_coolant(void);
void set_low_coolant_adc_fault(void);
void set_high_coolant_adc_fault(void);
void use_fallback_coolant_temp(void);

void update_coolant_from_samples(void)
{
    stock_r01 = coolant_sample_sum;
    shift_left_saturating_3();

    B = coolant_diagnostic_delay;
    if (time_since_start_f873 >= B) {
        B = coolant_adc_limits[0];
        if (stock_r1 < B) {
            set_low_coolant_adc_fault();
            return;
        }
        B = coolant_adc_limits[1];
        if (stock_r1 >= B) {
            set_high_coolant_adc_fault();
            return;
        }
    }

    convert_measured_coolant();
}

void convert_measured_coolant(void)
{
    stock_r3 = (uint8_t)(stock_r1 << 5);
    stock_r2 = stock_r1 >> 3;
    STOCK_DPTR = (uint16_t)coolant_transfer_raw;
    interp_curve_u8();
    ACC = ACC < COOLANT_TRANSFER_OFFSET ? 0 : ACC - COOLANT_TRANSFER_OFFSET;
    coolant_temperature_raw = ACC;
    measured_coolant_raw = ACC;
    coolant_adc_low = 0;
    coolant_adc_high = 0;

    B = ACC;
    if (coolant_rise_latched == 0 && coolant_rise_limits[1] < B) {
        /* The rise comparison uses subtraction modulo 256. */
        B -= startup_coolant_raw;
        if (coolant_rise_limits[0] < B)
            coolant_rise_latched = 1;
    }

    update_coolant_axis();
}

void set_low_coolant_adc_fault(void)
{
    coolant_adc_low = 1;
    coolant_adc_high = 0;
    use_fallback_coolant_temp();
}

void set_high_coolant_adc_fault(void)
{
    coolant_adc_low = 0;
    coolant_adc_high = 1;
    use_fallback_coolant_temp();
}

void use_fallback_coolant_temp(void)
{
    B = fallback_runtime >> 4;
    coolant_temperature_raw = coolant_fallback_temperature[B];
    update_coolant_axis();
}
