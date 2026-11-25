#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x07) idle_speed_trim_enabled;
static volatile __data __at (0x00) uint8_t bank_zero_r0;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __xdata __at (0xf84f) uint8_t idle_rpm_correction;
static volatile __xdata __at (0xf856) uint16_t adaptation_error_sum;
static volatile __xdata __at (0xf858) uint8_t adaptation_squared_error_lo;
static volatile __xdata __at (0xf859) uint8_t adaptation_squared_error_mid;
static volatile __xdata __at (0xf85a) uint8_t adaptation_squared_error_hi;
static volatile __xdata __at (0xf8be) uint8_t iac_adaptation_elapsed;

extern volatile __code const uint8_t iac_adaptation_delay_warm;
extern volatile __code const uint8_t iac_adaptation_delay_cold;
extern volatile __code const uint8_t idle_iac_cold_temp_limit;
extern volatile __code const uint8_t idle_error_spread_low;
extern volatile __code const uint8_t idle_error_spread_high;
extern volatile __code const int8_t idle_rpm_trim_min;
extern volatile __code const uint8_t idle_rpm_trim_max;

extern void negate_u16_register_pair(void);
extern void multiply_stock_u16_u8(void);
extern void shift_left_saturating_3(void);
extern void limit_stock_i8_by_sign(void);
extern void continue_iac_adaptation(void);
extern void finish_iac_adaptation_window(void);

void calculate_iac_error_spread(void);
void update_iac_rpm_trim(void);

void evaluate_iac_adaptation_window(void)
{
    B = iac_adaptation_delay_warm;
    if (coolant_temperature_raw <= idle_iac_cold_temp_limit) {
        B = iac_adaptation_delay_cold;
    }
    if (iac_adaptation_elapsed < B) {
        finish_iac_adaptation_window();
        return;
    }
    calculate_iac_error_spread();
}

void calculate_iac_error_spread(void)
{
    stock_r01 = adaptation_error_sum;
    if (stock_r1 & 0x80) {
        negate_u16_register_pair();
    }

    /* The mean approximation keeps only the high byte of the absolute
     * sum, doubles it and truncates it to a byte.
     */
    B = (uint8_t)(stock_r1 << 1);
    multiply_stock_u16_u8();

    /* Subtract the product R1:R0:B from the 24-bit sum of squared errors. */
    const uint16_t low = (uint16_t)adaptation_squared_error_lo - B;
    B = (uint8_t)low;
    const uint16_t middle = (uint16_t)adaptation_squared_error_mid
                          - stock_r0 - ((low >> 8) & 1);
    stock_r0 = (uint8_t)middle;
    const uint8_t high = adaptation_squared_error_hi
                       - stock_r1 - ((middle >> 8) & 1);
    if (high || (stock_r0 & 0xe0)) {
        stock_r1 = UINT8_MAX;
    } else {
        /* The stock load uses physical IRAM:00, regardless of PSW bank. */
        stock_r1 = bank_zero_r0;
        stock_r0 = B;
        shift_left_saturating_3();
    }
    update_iac_rpm_trim();
}

void update_iac_rpm_trim(void)
{
    if (!idle_speed_trim_enabled) {
        continue_iac_adaptation();
        return;
    }

    B = idle_error_spread_low;
    if (stock_r1 < B) {
        ACC = idle_rpm_correction - 1;
    } else {
        B = idle_error_spread_high;
        if (stock_r1 < B) {
            continue_iac_adaptation();
            return;
        }
        ACC = idle_rpm_correction + 2;
    }

    B = ACC;
    stock_r0 = (uint8_t)idle_rpm_trim_min;
    stock_r1 = idle_rpm_trim_max;
    ACC = B;
    limit_stock_i8_by_sign();
    idle_rpm_correction = ACC;
    continue_iac_adaptation();
}
