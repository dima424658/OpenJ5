#include <stdint.h>

enum {
    HOLD_IAC_TARGET = 0x20,
    HOLD_FUEL_STATE = 0x02,
};

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0x54) poststart_iac_enabled;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x5d) fuel_mode_flag_5d;
static __sbit __at (0x62) idle_transition_hold_62;
static __sbit __at (0x6e) air_conditioning_active;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x69) uint8_t airflow_for_iac;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __data __at (0x7b) uint8_t fuel_control_7b;
static volatile __xdata __at (0xf847) uint8_t idle_transition_rpm_1;
static volatile __xdata __at (0xf849) uint8_t idle_rpm_target;
static volatile __xdata __at (0xf84a) uint8_t idle_feedback_rpm_target;
static volatile __xdata __at (0xf869) uint8_t coast_work_f869;
static volatile __xdata __at (0xf97b) uint8_t fuel_decay_low_f97b;

extern __code const uint8_t idle_airflow_by_temp[39];
extern __code const uint8_t idle_ac_airflow_by_temp[39];
extern void apply_poststart_iac_airflow(void);
extern void update_idle_ignition_feedback(void);
extern void update_idle_iac_feedback(void);
extern void continue_after_idle_airflow(void);

void enter_poststart_idle(void);
void keep_idle_feedback(void);

void enter_regulated_idle(void)
{
    fuel_cut_phase = 0;
    idle_regulating = 1;
    if (poststart_iac_enabled) {
        enter_poststart_idle();
        return;
    }

    B = idle_rpm_target;
    ACC = engine_rpm_10;
    idle_feedback_rpm_target = ACC;
    if (ACC < B) {
        idle_feedback_rpm_target = idle_rpm_target;
        keep_idle_feedback();
        return;
    }

    if (idle_transition_hold_62) {
        B = idle_transition_rpm_1;
        if (engine_rpm_10 >= B) {
            idle_feedback_rpm_target = idle_transition_rpm_1;
        }
    }
    keep_idle_feedback();
}

void enter_poststart_idle(void)
{
    if (air_conditioning_active) {
        airflow_for_iac = idle_ac_airflow_by_temp[coolant_nearest_index];
    } else {
        airflow_for_iac = idle_airflow_by_temp[coolant_nearest_index];
    }

    if (!(ignition_and_airflow_control_7a & HOLD_IAC_TARGET)) {
        apply_poststart_iac_airflow();
        fuel_mode_flag_5d = 0;
        fuel_decay_low_f97b = 0;
        idle_regulating = 0;
        continue_after_idle_airflow();
        return;
    }
    idle_feedback_rpm_target = idle_rpm_target;
    keep_idle_feedback();
}

void keep_idle_feedback(void)
{
    if (!(fuel_control_7b & HOLD_FUEL_STATE)) {
        coast_work_f869 = 0;
    }
    fuel_mode_flag_5d = 0;
    fuel_decay_low_f97b = 0;
    update_idle_ignition_feedback();
    update_idle_iac_feedback();
    continue_after_idle_airflow();
}
