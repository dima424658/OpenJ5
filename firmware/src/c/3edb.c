#include "stock_registers.h"

static __sfr __at (0xe8) P4;
static __sfr __at (0xf0) B;
static __sbit __at (0x55) fuel_mode_flag_55;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x5c) fuel_mode_flag_5c;
static __sbit __at (0x5d) knock_zone_active;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x59) uint8_t rpm_nearest_bin;
static volatile __data __at (0x62) uint8_t requested_ignition;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __data __at (0x7b) uint8_t fuel_control_7b;
static volatile __data __at (0x7d) uint8_t oxygen_control_7d;
static volatile __xdata __at (0xf832) uint8_t mixture_rate_fraction;
static volatile __xdata __at (0xf833) uint8_t mixture_rate_output;
static volatile __xdata __at (0xf869) uint8_t coast_work_f869;
static volatile __xdata __at (0xf872) uint8_t start_exit_pending;
static volatile __xdata __at (0xf8af) uint8_t rpm_eighth_bin;

extern volatile __code const uint8_t startup_mixture;
extern volatile __code const uint8_t cold_start_coolant_threshold;
extern volatile __code const uint8_t cold_start_iac_max_position;
extern volatile __code const uint8_t iac_max_position;
extern __code const uint8_t cranking_ignition_raw[40];
extern __code const uint8_t throttle_quantization_q4[256];
extern volatile __code const uint8_t operating_zone_flags[256];

extern void clear_fuel_history_f839(void);
extern void apply_requested_mixture(void);
extern void update_idle_or_economy_mixture(void);

void reset_start_mode_3edb(void)
{
    fuel_mode_flag_5c = 0;
    knock_zone_active = 0;
    idle_regulating = 0;
    fuel_cut_phase = 0;

    if (!(fuel_control_7b & 2)) {
        coast_work_f869 = 0;
    }

    B = startup_mixture;
    mixture_rate_fraction = 0;
    mixture_rate_output = B;
}

void apply_cranking_outputs(void)
{
    if (!(ignition_and_airflow_control_7a & 4)) {
        B = rpm_eighth_bin;
        requested_ignition = cranking_ignition_raw[B];
    }

    if (!(ignition_and_airflow_control_7a & 0x20)) {
        if (coolant_temperature_raw <= cold_start_coolant_threshold) {
            iac_target = cold_start_iac_max_position;
        } else {
            iac_target = iac_max_position;
        }
    }

    clear_fuel_history_f839();
    reset_start_mode_3edb();
    ACC = start_exit_pending;
    if (!ACC || ((uint8_t)~P4 & 0x0f) == 0) {
        if (ACC) {
            start_exit_pending = 0;
        }
        fuel_mode_flag_55 = 1;
        fuel_mode_flag_56 = 1;
    }
    apply_requested_mixture();
}

void select_knock_regulation_zone(void)
{
    if (oxygen_control_7d & 4) {
        knock_zone_active = 0;
    } else {
        stock_r2 = ((throttle_quantization_q4[throttle_position] + 8) & 0xf0)
                   + rpm_nearest_bin;
        ACC = operating_zone_flags[stock_r2];
        knock_zone_active = (ACC & 1) || ((ACC & 2) && knock_zone_active);
    }
    update_idle_or_economy_mixture();
}
