#include <stdint.h>

enum {
    FUEL_CUT_WAITING = 1,
    FUEL_CUT_ACTIVE = 2,
    HOLD_BASE_IGNITION = 0x04,
    HOLD_FUEL_STATE = 0x02,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x06) fuel_always_enabled;
static __sbit __at (0x0c) speed_sensor_fitted;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x5d) fuel_mode_flag_5d;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x54) uint8_t engine_rpm_30;
static volatile __data __at (0x62) uint8_t base_ignition;
static volatile __data __at (0x63) uint8_t idle_ignition;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __data __at (0x6c) uint8_t fuel_cut_elapsed;
static volatile __data __at (0x7a) uint8_t ignition_control_7a;
static volatile __data __at (0x7b) uint8_t fuel_control_7b;
static volatile __xdata __at (0xf869) uint8_t coast_work_f869;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf97b) uint8_t fuel_decay_low_f97b;

extern volatile __code const uint8_t fuel_cut_coolant_threshold;
extern volatile __code const uint8_t fuel_cut_speed_threshold;
extern volatile __code const uint8_t fuel_cut_rpm_threshold;
extern volatile __code const uint8_t fuel_cut_delay_ticks;
extern void update_coast_fuel_cut(void);
extern void apply_idle_airflow(void);

void check_coast_fuel_cut_delay(void);
void begin_coast_fuel_cut(void);

void arm_coast_fuel_cut(void)
{
    fuel_cut_phase = FUEL_CUT_WAITING;
    idle_regulating = 0;
    fuel_cut_elapsed = 0;
    check_coast_fuel_cut_delay();
}

void check_coast_fuel_cut_delay(void)
{
    if (!(fuel_control_7b & HOLD_FUEL_STATE)) {
        coast_work_f869 = 0;
    }
    fuel_mode_flag_5d = 0;
    fuel_decay_low_f97b = 0;
    if (!(ignition_control_7a & HOLD_BASE_IGNITION)) {
        base_ignition = idle_ignition;
    }

    if (fuel_always_enabled) {
        goto apply_airflow;
    }
    B = fuel_cut_coolant_threshold;
    if (coolant_temperature_raw < B) {
        goto apply_airflow;
    }

    B = fuel_cut_rpm_threshold;
    if (engine_rpm_30 >= B || !speed_sensor_fitted || speed_sensor_fault) {
        goto advance_delay;
    }
    B = fuel_cut_speed_threshold;
    if (vehicle_speed >= B) {
        goto advance_delay;
    }

apply_airflow:
    apply_idle_airflow();
    return;

advance_delay:
    B = fuel_cut_delay_ticks;
    ++fuel_cut_elapsed;
    if (fuel_cut_elapsed < B) {
        goto apply_airflow;
    }
    begin_coast_fuel_cut();
}

void begin_coast_fuel_cut(void)
{
    fuel_cut_phase = FUEL_CUT_ACTIVE;
    idle_regulating = 0;
    fuel_cut_elapsed = 0;
    update_coast_fuel_cut();
}
