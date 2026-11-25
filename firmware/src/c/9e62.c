#include "stock_registers.h"

enum {
    IDLE_IGNITION_ERROR_SCALE = 8,
    HOLD_BASE_IGNITION = 0x04,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x6d) vehicle_moving;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x5d) uint8_t ignition_rpm_index;
static volatile __data __at (0x62) uint8_t base_ignition;
static volatile __data __at (0x63) uint8_t idle_ignition_target_raw;
static volatile __data __at (0x6d) int8_t idle_rpm_error;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __xdata __at (0xf844) uint8_t moving_ignition_f844;

extern volatile __code const uint8_t idle_ignition_deadband;
extern volatile __code const uint8_t idle_ignition_gain_nonnegative;
extern volatile __code const uint8_t idle_ignition_gain_negative;
extern volatile __code const uint8_t dynamic_ignition_temp_limit;
extern __code const int8_t minimum_ignition_offset_raw[32];
extern __code const int8_t maximum_ignition_offset_raw[32];

extern void abs_accumulator_i8(void);
extern void sign_extend_i8_register(void);
extern void multiply_stock_i16_u8(void);
extern void saturate_stock_i16_to_i8(void);
extern void limit_stock_i8_by_sign(void);
extern void prepare_idle_airflow_feedback(void);

void limit_and_store_idle_ignition(void);

void apply_idle_ignition_correction(void)
{
    if (ignition_and_airflow_control_7a & HOLD_BASE_IGNITION) {
        prepare_idle_airflow_feedback();
        return;
    }

    B = idle_ignition_deadband;
    ACC = (uint8_t)idle_rpm_error;
    stock_r0 = ACC;
    abs_accumulator_i8();
    if (ACC < B) {
        stock_r0 = 0;
    }
    sign_extend_i8_register();
    B = IDLE_IGNITION_ERROR_SCALE;
    multiply_stock_i16_u8();

    /* The 24-bit product is returned in R1:R0:B. Move the low 16 bits
     * to R1:R0 for the second multiplication.
     */
    stock_r01 = ((uint16_t)stock_r0 << 8) | B;
    ACC = stock_r1;
    if (ACC_SIGN) {
        B = idle_ignition_gain_negative;
    } else {
        B = idle_ignition_gain_nonnegative;
    }
    multiply_stock_i16_u8();
    /* This helper takes R1:R0, leaving the low product byte out. */
    saturate_stock_i16_to_i8();
    limit_and_store_idle_ignition();
}

void limit_and_store_idle_ignition(void)
{
    B = ACC;
    stock_r0 = (uint8_t)minimum_ignition_offset_raw[ignition_rpm_index];
    stock_r1 = (uint8_t)maximum_ignition_offset_raw[ignition_rpm_index];
    ACC = B;
    limit_stock_i8_by_sign();
    B = ACC;

    if (vehicle_moving && coolant_temperature_raw > dynamic_ignition_temp_limit) {
        base_ignition = moving_ignition_f844;
    } else {
        base_ignition = idle_ignition_target_raw + B;
    }
    prepare_idle_airflow_feedback();
}
