#include <stdint.h>

enum { FUEL_CUT_ACTIVE_PHASE = 2 };

static __sfr __at (0xf0) B;
static __sbit __at (0x06) fuel_always_enabled;
static __sbit __at (0x0c) speed_sensor_fitted;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x62) idle_transition_hold_62;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __xdata __at (0xf847) uint8_t idle_transition_rpm_1;
static volatile __xdata __at (0xf848) uint8_t idle_transition_rpm_2;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;

extern __code const uint8_t open_throttle_by_rpm[32];
extern __code const uint8_t closed_throttle_by_rpm[32];
extern volatile __code const uint8_t fuel_cut_coolant_threshold;
extern volatile __code const uint8_t fuel_cut_speed_threshold;

extern void leave_idle_for_load(void);
extern void keep_idle_feedback(void);
extern void enter_regulated_idle(void);
extern void run_airflow_control(void);
extern void arm_coast_fuel_cut(void);
extern void check_coast_fuel_cut_delay(void);
extern void begin_coast_fuel_cut(void);
extern void update_coast_fuel_cut(void);

void update_idle_or_cut_from_coast(void);
void try_enter_regulated_idle(void);

void select_idle_or_fuel_cut(void)
{
    if (!idle_regulating) {
        update_idle_or_cut_from_coast();
        return;
    }

    B = open_throttle_by_rpm[rpm_nearest_half_bin];
    if (throttle_position >= B) {
        leave_idle_for_load();
        return;
    }
    if (idle_transition_hold_62) {
        goto feedback;
    }
    B = idle_transition_rpm_2;
    if (engine_rpm_10 < B) {
feedback:
        keep_idle_feedback();
        return;
    }

    if (fuel_always_enabled) {
        goto arm_cut;
    }
    B = fuel_cut_coolant_threshold;
    if (coolant_temperature_raw < B) {
        goto arm_cut;
    }
    if (!speed_sensor_fitted || speed_sensor_fault) {
        goto begin_cut;
    }
    B = fuel_cut_speed_threshold;
    if (vehicle_speed < B) {
arm_cut:
        arm_coast_fuel_cut();
        return;
    }
begin_cut:
    begin_coast_fuel_cut();
}

void update_idle_or_cut_from_coast(void)
{
    if (fuel_cut_phase == 0) {
        try_enter_regulated_idle();
        return;
    }

    B = open_throttle_by_rpm[rpm_nearest_half_bin];
    if (throttle_position >= B) {
        leave_idle_for_load();
        return;
    }
    B = idle_transition_rpm_1;
    if (engine_rpm_10 < B) {
        enter_regulated_idle();
        return;
    }
    if (fuel_cut_phase < FUEL_CUT_ACTIVE_PHASE) {
        check_coast_fuel_cut_delay();
        return;
    }
    update_coast_fuel_cut();
}

void try_enter_regulated_idle(void)
{
    B = closed_throttle_by_rpm[rpm_nearest_half_bin];
    if (throttle_position >= B) {
        run_airflow_control();
        return;
    }
    if (idle_transition_hold_62) {
        goto enter_idle;
    }
    B = idle_transition_rpm_2;
    if (engine_rpm_10 < B) {
enter_idle:
        enter_regulated_idle();
        return;
    }
    arm_coast_fuel_cut();
}
