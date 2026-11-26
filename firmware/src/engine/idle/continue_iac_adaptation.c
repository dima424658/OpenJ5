#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xaf) interrupts_enabled;
static volatile __data __at (0x01) uint8_t bank_zero_r1;
static volatile __data __at (0x03) uint8_t bank_zero_r3;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __xdata __at (0xf845) uint16_t iac_feedback_position_q4;
static volatile __xdata __at (0xf852) uint16_t adaptation_position_sum;
static volatile __xdata __at (0xf854) uint16_t adaptation_airflow_sum;

extern volatile __code const uint8_t iac_airflow_gain_q7;
extern volatile __code const uint8_t iac_max_airflow;
extern volatile __code const int8_t iac_initial_offset;
extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t idle_iac_cold_temp_limit;
extern volatile __code const uint8_t iac_cold_max_position;
extern void shift_left_saturating_1(void);
extern void clamp_stock_u8(void);
extern void update_iac_adaptation_ready(void);

void continue_iac_adaptation(void)
{
    stock_r01 = adaptation_position_sum;
    shift_left_saturating_1();
    stock_r2 = bank_zero_r1;
    stock_r3 = iac_max_airflow;

    B = iac_airflow_gain_q7;
    stock_r01 = adaptation_airflow_sum;
    shift_left_saturating_1();
    /* The high byte of the doubled sum is the mean of 128 samples. */
    ACC = stock_r1;
    stock_r0 = 0;
    stock_r1 = bank_zero_r3;
    clamp_stock_u8();
    B = (uint8_t)(((uint16_t)ACC * B) >> 7);
    B += (uint8_t)iac_initial_offset;

    stock_r0 = 0;
    stock_r1 = iac_max_position;
    if (coolant_temperature_raw <= idle_iac_cold_temp_limit) {
        stock_r1 = iac_cold_max_position;
    }
    ACC = B;
    clamp_stock_u8();

    interrupts_enabled = 0;
    iac_target = ACC;
    iac_position = ACC;
    interrupts_enabled = 1;

    B = ACC;
    stock_r01 = iac_feedback_position_q4;
    /* Replace the whole-step part while retaining the substep fraction. */
    stock_r1 = B >> 4;
    B = (uint8_t)(B << 4);
    stock_r0 = (stock_r0 & 0x0f) + B;
    iac_feedback_position_q4 = stock_r01;
    update_iac_adaptation_ready();
}
