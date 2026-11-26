#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xf7) B_SIGN;
static __sbit __at (0x6e) air_conditioning_active;
static volatile __data __at (0x48) uint8_t measured_airflow;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x5f) uint8_t iac_stiffness_index;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x6d) int8_t idle_rpm_error;
static volatile __xdata __at (0xf845) uint16_t iac_feedback_position_q4;
static volatile __xdata __at (0xf8a0) uint8_t iac_fraction_lo;
static volatile __xdata __at (0xf8a1) uint8_t iac_fraction_hi;

extern volatile __code const uint8_t iac_feedback_proportional_gain;
extern volatile __code const uint8_t iac_feedback_integral_gain;
extern volatile __code const uint8_t iac_integral_error_limit;
extern __code const uint8_t iac_feedback_stiffness[256];
extern volatile __code const uint8_t iac_min_airflow;
extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t idle_iac_cold_temp_limit;
extern volatile __code const uint8_t iac_cold_max_position;
extern volatile __code const uint8_t iac_min_position;
extern volatile __code const uint8_t iac_ac_min_position;

extern void limit_stock_i8_by_sign(void);
extern void sign_extend_i8_register(void);
extern void multiply_stock_i16_u8(void);
extern void add_u16_register_pairs(void);
extern void minimum_u16_register_pair(void);
extern void maximum_u16_register_pair(void);
extern void shift_left_saturating_4(void);

void scale_idle_iac_feedback(void);
void apply_idle_iac_position_delta(void);

/* B holds the signed change in speed error from the preparation block. */
void apply_idle_airflow_feedback(void)
{
    stock_r0 = iac_feedback_proportional_gain;
    /* Correct the high byte of the unsigned product for a negative input. */
    stock_r1 = B_SIGN ? (uint8_t)-stock_r0 : 0;
    const uint16_t proportional = (uint16_t)stock_r0 * B;
    stock_r2 = (uint8_t)proportional;
    stock_r3 = (uint8_t)(proportional >> 8) + stock_r1;

    ACC = iac_integral_error_limit;
    stock_r1 = ACC;
    stock_r0 = (uint8_t)-ACC;
    ACC = (uint8_t)idle_rpm_error;
    limit_stock_i8_by_sign();
    stock_r0 = ACC;
    sign_extend_i8_register();
    B = iac_feedback_integral_gain;
    multiply_stock_i16_u8();
    /* Add the proportional product to the high word of R1:R0:B. */
    add_u16_register_pairs();
    scale_idle_iac_feedback();
}

void scale_idle_iac_feedback(void)
{
    /* Multiply the high word and the low byte by the same stiffness.
     * Keep the low 16 bits in XDATA for the next control cycle.
     */
    stock_r2 = B;
    B = iac_feedback_stiffness[iac_stiffness_index];
    multiply_stock_i16_u8();
    const uint16_t high_fraction = (uint16_t)iac_fraction_hi + B;
    iac_fraction_hi = (uint8_t)high_fraction;
    stock_r01 += high_fraction >> 8;

    B = iac_feedback_stiffness[iac_stiffness_index];
    const uint16_t low_product = (uint16_t)stock_r2 * B;
    const uint16_t low_fraction = (uint16_t)iac_fraction_lo + (uint8_t)low_product;
    iac_fraction_lo = (uint8_t)low_fraction;
    const uint16_t carried_fraction = (uint16_t)iac_fraction_hi + (low_product >> 8)
                                   + (low_fraction >> 8);
    iac_fraction_hi = (uint8_t)carried_fraction;
    stock_r01 += carried_fraction >> 8;
    apply_idle_iac_position_delta();
}

void apply_idle_iac_position_delta(void)
{
    B = iac_min_airflow;
    if (measured_airflow < B && (stock_r1 & 0x80)) {
        return;
    }
    stock_r23 = iac_feedback_position_q4;
    add_u16_register_pairs();
    if (stock_r1 & 0x80) {
        stock_r01 = 0;
    }

    B = iac_max_position;
    if (coolant_temperature_raw <= idle_iac_cold_temp_limit) {
        B = iac_cold_max_position;
    }
    stock_r23 = (uint16_t)B << 4;
    minimum_u16_register_pair();

    if (air_conditioning_active) {
        B = iac_ac_min_position;
    } else {
        B = iac_min_position;
    }
    stock_r23 = (uint16_t)B << 4;
    maximum_u16_register_pair();
    iac_feedback_position_q4 = stock_r01;
    shift_left_saturating_4();
    iac_target = stock_r1;
}
