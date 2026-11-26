#include "stock_registers.h"

static __sfr __at (0xe8) P4;
static __sfr __at (0xf0) B;
static __sfr __at (0xf6) sfr_f6;
static __sbit __at (0x31) event_flag_31;
static __sbit __at (0x3e) ignition_coil_order;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x4d) shutdown_pending;
static __sbit __at (0x4e) oxygen_heater_active;
static __sbit __at (0x51) startup_flag_51;
static __sbit __at (0x53) startup_flag_53;
static __sbit __at (0x54) poststart_iac_enabled;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x58) crank_stopped;
static __sbit __at (0x6b) service_link_ready_6b;
static __sbit __at (0x73) service_pump_output;
static __sbit __at (0xfe) oxygen_heater_output_high;

static volatile __data __at (0x10) uint8_t cranking_compare_bank[8];
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4d) uint8_t injection_phase;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x62) uint8_t requested_ignition;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __data __at (0x7c) uint8_t output_mask_7c;

static volatile __xdata __at (0xf808) uint8_t mass_airflow[2];
static volatile __xdata __at (0xf80a) uint8_t startup_coolant_temperature_raw;
static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf828) uint8_t fuel_pump_prime_elapsed;
static volatile __xdata __at (0xf82c) uint8_t measured_air_charge[2];
static volatile __xdata __at (0xf82e) uint8_t cylinder_charge[2];
static volatile __xdata __at (0xf837) uint8_t cranking_fuel[2];
static volatile __xdata __at (0xf841) uint8_t sensor_sum_state_f841[2];
static volatile __xdata __at (0xf84a) uint8_t idle_feedback_rpm_target;
static volatile __xdata __at (0xf89a) uint8_t poststart_mixture_seconds;
static volatile __xdata __at (0xf89b) uint8_t poststart_fuel_floor[2];
static volatile __xdata __at (0xf89d) uint8_t fuel_film_events;
static volatile __xdata __at (0xf89e) uint8_t cranking_events;
static volatile __xdata __at (0xf8af) uint8_t rpm_eighth_bin;
static volatile __xdata __at (0xf8c5) uint8_t oxygen_ready_count;
static volatile __xdata __at (0xf94c) uint8_t poststart_iac_upper_offset;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;

extern volatile __code const uint8_t fuel_pump_prime_seconds;
extern volatile __code const uint8_t startup_injection_phase;
extern volatile __code const uint8_t startup_measured_charge_low;
extern volatile __code const uint8_t startup_measured_charge_high;
extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t poststart_mixture_duration;
extern __code const uint8_t poststart_iac_upper_offset_8c00[39];
extern __code const uint8_t cranking_ignition_raw[40];

extern void clear_fuel_history_f839(void);
extern void reset_start_mode_3edb(void);
extern void finish_ignition_corrections(void);
extern void begin_asynchronous_cranking(void);
extern void advance_cranking_exit(void);

void prepare_cranking_state(void);
void prepare_cranking_ignition(void);

void continue_engine_stop_cycle(void)
{
    if (shutdown_pending) {
        prepare_cranking_state();
        return;
    }
    if (fuel_mode_flag_56) {
        if (crank_stopped) {
            prepare_cranking_state();
        } else {
            advance_cranking_exit();
        }
        return;
    }
    if (!crank_stopped) {
        begin_asynchronous_cranking();
        return;
    }

    ACC = output_mask_7c & 0x0f;
    sfr_f6 &= 0xf0;
    if (!ACC) {
        P4 |= 0x0f;
    }
    oxygen_heater_output_high = 1;
    oxygen_heater_active = 0;
    oxygen_diagnostic_status &= 0xfd;

    if (output_override_flags & 8) {
        relay_output_latch = (relay_output_latch & 0xbf) | (service_pump_output ? 0x40 : 0);
    } else if (service_link_ready_6b) {
        B = fuel_pump_prime_seconds;
        ACC = fuel_pump_prime_elapsed;
        if (tick_1s && ACC != UINT8_MAX) {
            ++ACC;
        }
        fuel_pump_prime_elapsed = ACC;
        relay_output_latch = (relay_output_latch & 0xbf) | (ACC < B ? 0x40 : 0);
    }
    prepare_cranking_ignition();
}

void prepare_cranking_state(void)
{
    fuel_mode_flag_56 = 0;
    event_flag_31 = 0;
    startup_flag_53 = 1;
    poststart_iac_enabled = 1;
    startup_flag_51 = 1;
    poststart_iac_upper_offset = poststart_iac_upper_offset_8c00[coolant_nearest_index];

    oxygen_diagnostic_status = 0;
    oxygen_ready_count = 0;
    if (!(ignition_and_airflow_control_7a & 2)) {
        injection_phase = startup_injection_phase;
    }

    measured_air_charge[0] = startup_measured_charge_low;
    measured_air_charge[1] = startup_measured_charge_high;
    iac_target = iac_max_position;
    idle_feedback_rpm_target = 0;
    cranking_events = 0;
    engine_rpm_10 = 0;
    sensor_sum_state_f841[0] = 0;
    sensor_sum_state_f841[1] = 0;
    cranking_fuel[0] = 0;
    cranking_fuel[1] = 0;
    poststart_fuel_floor[0] = 0;
    poststart_fuel_floor[1] = 0;
    cylinder_charge[0] = 0;
    cylinder_charge[1] = 0;
    mass_airflow[0] = 0;
    mass_airflow[1] = 0;
    fuel_film_events = 0;
    clear_fuel_history_f839();
    reset_start_mode_3edb();
    prepare_cranking_ignition();
}

void prepare_cranking_ignition(void)
{
    startup_coolant_temperature_raw = coolant_temperature_raw;
    if (!(ignition_and_airflow_control_7a & 4)) {
        B = rpm_eighth_bin;
        requested_ignition = cranking_ignition_raw[B];
    }

    stock_r0 = 0x80;
    if (!ignition_coil_order) {
        for (uint8_t i = 0; i < 4; ++i) {
            cranking_compare_bank[i] = *(volatile __idata uint8_t *)stock_r0++;
        }
    }
    for (uint8_t i = 4; i < 8; ++i) {
        cranking_compare_bank[i] = *(volatile __idata uint8_t *)stock_r0++;
    }
    if (ignition_coil_order) {
        for (uint8_t i = 0; i < 4; ++i) {
            cranking_compare_bank[i] = *(volatile __idata uint8_t *)stock_r0++;
        }
    }
    poststart_mixture_seconds = poststart_mixture_duration;
    finish_ignition_corrections();
}
