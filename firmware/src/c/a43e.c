#include "stock_registers.h"

static __sfr __at (0xf0) B;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x69) uint8_t airflow_for_iac;
static volatile __xdata __at (0xf848) uint8_t idle_transition_rpm_2;
static volatile __xdata __at (0xf976) uint8_t iac_offset_elapsed;
static volatile __xdata __at (0xf977) uint8_t iac_airflow_offset;

extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t iac_airflow_gain_q7;
extern volatile __code const uint8_t iac_min_airflow;
extern volatile __code const uint8_t iac_max_airflow;
extern volatile __code const uint8_t closed_throttle_position;
extern volatile __code const uint8_t iac_adaptation_period;
extern volatile __code const int8_t iac_adaptation_min_offset;
extern volatile __code const int8_t iac_adaptation_max_offset;
extern volatile __code const int8_t iac_open_throttle_offset;

extern void clamp_stock_u8(void);
extern void limit_stock_i8_by_sign(void);

void convert_iac_airflow_to_position(void);

void apply_iac_airflow_target(void)
{
    stock_r0 = (uint8_t)iac_adaptation_min_offset;
    stock_r1 = (uint8_t)iac_adaptation_max_offset;
    B = closed_throttle_position;
    if (throttle_position >= B) {
        goto open_throttle;
    }
    B = idle_transition_rpm_2;
    if (engine_rpm_10 >= B) {
        ACC = stock_r1;
        goto reset_offset;
    }

    B = iac_adaptation_period;
    ACC = iac_offset_elapsed + 1;
    if (ACC >= B) {
        iac_offset_elapsed = 0;
        ACC = iac_airflow_offset - 1;
        limit_stock_i8_by_sign();
        iac_airflow_offset = ACC;
        goto convert;
    }
    iac_offset_elapsed = ACC;
    goto convert;

open_throttle:
    B = (uint8_t)iac_open_throttle_offset;
    ACC = stock_r1 + B;
reset_offset:
    iac_airflow_offset = ACC;
    iac_offset_elapsed = 0;
convert:
    convert_iac_airflow_to_position();
}

void convert_iac_airflow_to_position(void)
{
    B = iac_airflow_gain_q7;
    stock_r0 = iac_min_airflow;
    stock_r1 = iac_max_airflow;
    ACC = airflow_for_iac;
    clamp_stock_u8();

    /* Narrow the shifted product and the sum to one byte. */
    B = (uint8_t)(((uint16_t)ACC * B) >> 7);
    B += iac_airflow_offset;
    stock_r0 = 0;
    stock_r1 = iac_max_position;
    ACC = B;
    clamp_stock_u8();
    iac_target = ACC;
}
