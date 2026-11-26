#include "stock_registers.h"

enum {
    HOLD_IAC_TARGET = 0x20,
    IAC_ADAPTATION_WINDOW_SAMPLES = 128,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x49) adaptation_clock_tick_49;
static __sbit __at (0x61) was_idle_regulating;
static volatile __data __at (0x48) uint8_t measured_airflow;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __data __at (0x6d) int8_t idle_rpm_error;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __xdata __at (0xf84a) uint8_t idle_feedback_rpm_target;
static volatile __xdata __at (0xf84b) uint8_t adaptation_rpm_target;
static volatile __xdata __at (0xf851) uint8_t adaptation_sample_count;
static volatile __xdata __at (0xf852) uint16_t adaptation_position_sum;
static volatile __xdata __at (0xf854) uint16_t adaptation_airflow_sum;
static volatile __xdata __at (0xf856) uint16_t adaptation_error_sum;
static volatile __xdata __at (0xf858) uint8_t adaptation_squared_error_lo;
static volatile __xdata __at (0xf859) uint8_t adaptation_squared_error_mid;
static volatile __xdata __at (0xf85a) uint8_t adaptation_squared_error_hi;
static volatile __xdata __at (0xf8be) uint8_t iac_adaptation_elapsed;

extern void clear_iac_history(void);
extern void sign_extend_i24_register_triple(void);
extern void add_u16_register_pairs(void);
extern void abs_accumulator_i8(void);
extern void evaluate_iac_adaptation_window(void);

void prepare_iac_adaptation_window(void);
void accumulate_iac_adapt_sample(void);

void update_idle_iac_feedback(void)
{
    if (ignition_and_airflow_control_7a & HOLD_IAC_TARGET) {
        return;
    }
    prepare_iac_adaptation_window();
}

void prepare_iac_adaptation_window(void)
{
    B = idle_feedback_rpm_target;
    ACC = adaptation_rpm_target;
    if (!was_idle_regulating || ACC != B) {
        adaptation_rpm_target = idle_feedback_rpm_target;
        iac_adaptation_elapsed = 0;
        clear_iac_history();
    }
    if (adaptation_clock_tick_49) {
        ACC = iac_adaptation_elapsed + 1;
        if (ACC != 0) {
            iac_adaptation_elapsed = ACC;
        }
    }
    accumulate_iac_adapt_sample();
}

void accumulate_iac_adapt_sample(void)
{
    stock_r01 = adaptation_error_sum;
    stock_r2 = (uint8_t)idle_rpm_error;
    sign_extend_i24_register_triple();
    add_u16_register_pairs();
    adaptation_error_sum = stock_r01;

    ACC = (uint8_t)idle_rpm_error;
    abs_accumulator_i8();
    B = ACC;
    const uint16_t squared_error = (uint16_t)ACC * B;
    const uint16_t low_sum = (uint16_t)adaptation_squared_error_lo + (uint8_t)squared_error;
    adaptation_squared_error_lo = (uint8_t)low_sum;
    const uint16_t middle_sum = (uint16_t)adaptation_squared_error_mid
                              + (squared_error >> 8) + (low_sum >> 8);
    adaptation_squared_error_mid = (uint8_t)middle_sum;
    adaptation_squared_error_hi += (uint8_t)(middle_sum >> 8);

    adaptation_position_sum += iac_position;
    adaptation_airflow_sum += measured_airflow;
    ACC = adaptation_sample_count + 1;
    adaptation_sample_count = ACC;
    if (ACC != IAC_ADAPTATION_WINDOW_SAMPLES) {
        return;
    }
    if (ignition_and_airflow_control_7a & HOLD_IAC_TARGET) {
        return;
    }
    evaluate_iac_adaptation_window();
}
