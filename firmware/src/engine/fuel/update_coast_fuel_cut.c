#include <stdint.h>

enum {
    COAST_IGNITION_DELAY = 2,
    LOW_SPEED_IGNITION_DELAY = 4,
    HOLD_BASE_IGNITION = 0x04,
    HOLD_IAC_TARGET = 0x20,
    HOLD_FUEL_STATE = 0x02,
};

static __sfr __at (0xf0) B;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __data __at (0x62) uint8_t base_ignition;
static volatile __data __at (0x63) uint8_t idle_ignition;
static volatile __data __at (0x69) uint8_t airflow_for_iac;
static volatile __data __at (0x6a) uint8_t idle_airflow_request;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __data __at (0x6c) uint8_t fuel_cut_elapsed;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __data __at (0x7b) uint8_t fuel_control_7b;
static volatile __xdata __at (0xf869) uint8_t coast_work_f869;

extern volatile __code const uint8_t fuel_cut_ignition_delay_ticks;
extern __code const int8_t fuel_cut_ignition_half_degrees[32];
extern void apply_iac_airflow_target(void);
extern void check_fuel_cut_recovery(void);

void update_coast_fuel_cut(void)
{
    airflow_for_iac = idle_airflow_request;
    if (!(ignition_and_airflow_control_7a & HOLD_IAC_TARGET)) {
        apply_iac_airflow_target();
    }
    if (!(fuel_control_7b & HOLD_FUEL_STATE)) {
        coast_work_f869 = 0;
    }
    if (!(ignition_and_airflow_control_7a & HOLD_BASE_IGNITION)) {
        base_ignition = idle_ignition;
    }

    const uint8_t phase = fuel_cut_phase;
    if (phase == COAST_IGNITION_DELAY || phase == LOW_SPEED_IGNITION_DELAY) {
        B = fuel_cut_ignition_delay_ticks;
        ++fuel_cut_elapsed;
        if (fuel_cut_elapsed < B) {
            goto check_recovery;
        }
        ++fuel_cut_phase;
    }
    if (!(ignition_and_airflow_control_7a & HOLD_BASE_IGNITION)) {
        base_ignition = (uint8_t)fuel_cut_ignition_half_degrees[rpm_nearest_half_bin];
    }

check_recovery:
    check_fuel_cut_recovery();
}
