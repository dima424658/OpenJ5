#include <stdint.h>

enum {
    COAST_DELAY_PHASE = 2,
    COAST_TIMING_PHASE = 3,
    LOW_SPEED_PHASE_OFFSET = 2,
    HOLD_IAC_TARGET = 0x20,
    FUEL_TRANSITION_EVENTS = 4,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x0c) speed_sensor_fitted;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x55) fuel_mode_flag_55;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x5b) deceleration_fuel_cut;
static __sbit __at (0x5c) fuel_mode_flag_5c;
static __sbit __at (0x5d) fuel_mode_flag_5d;
static volatile __data __at (0x54) uint8_t engine_rpm_30;
static volatile __data __at (0x69) uint8_t airflow_for_iac;
static volatile __data __at (0x6a) uint8_t idle_airflow_request;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf8b1) uint8_t fuel_transition_event_count;
static volatile __xdata __at (0xf97b) uint8_t fuel_decay_low_f97b;

extern volatile __code const uint8_t fuel_cut_speed_threshold;
extern volatile __code const uint8_t fuel_resume_rpm_threshold;
extern volatile __code const uint8_t fuel_cut_rpm_threshold;
extern void update_fuel_cut_mixture(void);
extern void clear_fuel_history_f839(void);
extern void finish_ignition_corrections(void);
extern void continue_after_idle_airflow(void);
extern void apply_iac_airflow_target(void);

void apply_deceleration_fuel_cut(void);
void apply_idle_airflow(void);

void check_fuel_cut_recovery(void)
{
    if (!speed_sensor_fitted || speed_sensor_fault) {
        goto keep_cut;
    }

    const uint8_t phase = fuel_cut_phase;
    if (phase == COAST_DELAY_PHASE || phase == COAST_TIMING_PHASE) {
        B = fuel_cut_speed_threshold;
        if (vehicle_speed >= B) {
            goto keep_cut;
        }
        B = fuel_resume_rpm_threshold;
        if (engine_rpm_30 >= B) {
            goto keep_cut;
        }
        fuel_cut_phase += LOW_SPEED_PHASE_OFFSET;
        apply_idle_airflow();
        return;
    }

    B = fuel_cut_speed_threshold;
    if (vehicle_speed < B) {
        B = fuel_cut_rpm_threshold;
        if (engine_rpm_30 < B) {
            apply_idle_airflow();
            return;
        }
    }
    fuel_cut_phase -= LOW_SPEED_PHASE_OFFSET;

keep_cut:
    apply_deceleration_fuel_cut();
}

void apply_deceleration_fuel_cut(void)
{
    fuel_mode_flag_5c = 0;
    fuel_mode_flag_5d = 0;
    fuel_decay_low_f97b = 0;
    update_fuel_cut_mixture();

    deceleration_fuel_cut = 1;
    fuel_mode_flag_55 = 0;
    fuel_mode_flag_56 = 1;
    clear_fuel_history_f839();
    fuel_transition_event_count = FUEL_TRANSITION_EVENTS;
    finish_ignition_corrections();
}

void apply_idle_airflow(void)
{
    airflow_for_iac = idle_airflow_request;
    if (!(ignition_and_airflow_control_7a & HOLD_IAC_TARGET)) {
        apply_iac_airflow_target();
    }
    continue_after_idle_airflow();
}
