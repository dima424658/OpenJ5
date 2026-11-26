#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xf7) B_SIGN;
static __sbit __at (0x54) poststart_iac_enabled;
static volatile __data __at (0x00) uint8_t bank_zero_r0;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x69) uint8_t airflow_for_iac;
static volatile __xdata __at (0xf94b) uint8_t poststart_iac_elapsed;
static volatile __xdata __at (0xf94c) uint8_t poststart_iac_offset;

extern __code const int8_t poststart_iac_upper_offset_8c00[39];
extern __code const int8_t poststart_iac_lower_offset_8c27[39];
extern __code const uint8_t poststart_iac_period_8c4e[39];
extern volatile __code const uint8_t cold_start_iac_max_position;
extern volatile __code const uint8_t cold_start_coolant_threshold;
extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t iac_airflow_gain_q7;
extern volatile __code const uint8_t iac_min_airflow;
extern volatile __code const uint8_t iac_max_airflow;

extern void clamp_stock_u8(void);
extern void limit_stock_i8_by_sign(void);

void reset_poststart_iac_offset(void);
void convert_poststart_iac_airflow(void);

void apply_poststart_iac_airflow(void)
{
    stock_r0 = (uint8_t)poststart_iac_lower_offset_8c27[coolant_nearest_index];
    stock_r1 = (uint8_t)poststart_iac_upper_offset_8c00[coolant_nearest_index];
    B = poststart_iac_period_8c4e[coolant_nearest_index];

    ACC = poststart_iac_elapsed + 1;
    if (ACC >= B) {
        poststart_iac_elapsed = 0;
        ACC = poststart_iac_offset - 1;
        /* CJNE in the stock image reads IRAM:00 even in another bank. */
        if (ACC == bank_zero_r0) {
            poststart_iac_enabled = 0;
        }
        B = stock_r0;
        if (B_SIGN) {
            limit_stock_i8_by_sign();
        } else {
            clamp_stock_u8();
        }
        poststart_iac_offset = ACC;
    } else {
        poststart_iac_elapsed = ACC;
    }

    convert_poststart_iac_airflow();
}

// Separate entry at A524 uses an incoming offset in A.
void reset_poststart_iac_offset(void)
{
    poststart_iac_offset = ACC;
    poststart_iac_elapsed = 0;
    convert_poststart_iac_airflow();
}

void convert_poststart_iac_airflow(void)
{
    B = iac_airflow_gain_q7;
    stock_r0 = iac_min_airflow;
    stock_r1 = iac_max_airflow;
    ACC = airflow_for_iac;
    clamp_stock_u8();

    B = (uint8_t)(((uint16_t)ACC * B) >> 7);
    B += poststart_iac_offset;
    stock_r0 = 0;
    if (coolant_temperature_raw <= cold_start_coolant_threshold) {
        stock_r1 = cold_start_iac_max_position;
    } else {
        stock_r1 = iac_max_position;
    }
    ACC = B;
    clamp_stock_u8();
    iac_target = ACC;
}
