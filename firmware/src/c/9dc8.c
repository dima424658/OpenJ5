#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x61) was_idle_regulating;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x6d) int8_t idle_rpm_error;
static volatile __xdata __at (0xf845) uint16_t iac_feedback_position_q4;
static volatile __xdata __at (0xf847) uint8_t idle_transition_rpm_1;
static volatile __xdata __at (0xf849) uint8_t idle_rpm_target;
static volatile __xdata __at (0xf84a) uint8_t idle_feedback_rpm_target;
static volatile __xdata __at (0xf84c) int8_t previous_idle_rpm_error;
static volatile __xdata __at (0xf84e) uint8_t idle_rpm_ramp_elapsed;
static volatile __xdata __at (0xf8a0) uint8_t idle_feedback_state_f8a0;
static volatile __xdata __at (0xf8a1) uint8_t idle_feedback_state_f8a1;

extern volatile __code const uint8_t idle_rpm_ramp_period_above;
extern volatile __code const uint8_t idle_rpm_ramp_period_below;
extern void shift_left_saturating_4(void);
extern void apply_idle_ignition_correction(void);

void calculate_idle_rpm_error(void);

void update_idle_ignition_feedback(void)
{
    if (!was_idle_regulating) {
        idle_feedback_state_f8a0 = 0;
        idle_feedback_state_f8a1 = 0;
        idle_rpm_ramp_elapsed = 0;
        calculate_idle_rpm_error();
        return;
    }

    B = idle_rpm_target;
    if (idle_feedback_rpm_target < B) {
        idle_feedback_rpm_target = B;
    }
    B = idle_transition_rpm_1;
    if (idle_feedback_rpm_target >= B) {
        B = idle_rpm_ramp_period_above;
    } else {
        B = idle_rpm_ramp_period_below;
    }

    ACC = idle_rpm_ramp_elapsed + 1;
    idle_rpm_ramp_elapsed = ACC;
    if (ACC >= B) {
        idle_rpm_ramp_elapsed = 0;
        B = idle_rpm_target;
        ACC = idle_feedback_rpm_target - 1;
        idle_feedback_rpm_target = ACC;
        if (ACC < B) {
            idle_feedback_rpm_target = B;
        }
    }
    calculate_idle_rpm_error();
}

void calculate_idle_rpm_error(void)
{
    int16_t error = (int16_t)idle_feedback_rpm_target - engine_rpm_10;
    if (error > INT8_MAX) {
        error = INT8_MAX;
    } else if (error < INT8_MIN) {
        error = INT8_MIN;
    }
    idle_rpm_error = (int8_t)error;

    if (!was_idle_regulating) {
        previous_idle_rpm_error = idle_rpm_error;
        stock_r01 = iac_target;
        shift_left_saturating_4();
        iac_feedback_position_q4 = stock_r01;
    }
    apply_idle_ignition_correction();
}
