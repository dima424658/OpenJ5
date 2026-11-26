#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr __at (0xf2) sfr_f2;
static __sfr __at (0xf3) sfr_f3;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x00) oxygen_sensor_configured;
static __sbit __at (0x01) evap_fitted;
static __sbit __at (0x1c) oxygen_adc_low;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x24) oxygen_adc_high;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x26) maf_adc_high;
static __sbit __at (0x2b) oxygen_heater_fault;
static __sbit __at (0x2c) oxygen_adaptation_fault_2c;
static __sbit __at (0x2d) oxygen_adaptation_fault_2d;
static __sbit __at (0x4a) one_second_tick;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x5a) at_or_above_power_boundary;
static __sbit __at (0x5c) fuel_mode_flag_5c;
static __sbit __at (0x5e) purge_active;
static __sbit __at (0x5f) learning_sweep_active;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __data __at (0x62) uint8_t requested_ignition;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __data __at (0x7b) uint8_t fuel_control_7b;
static volatile __data __at (0x7d) uint8_t oxygen_control_7d;
static volatile __xdata __at (0xf806) uint8_t purge_duty;
static volatile __xdata __at (0xf807) uint8_t engine_run_seconds;
static volatile __xdata __at (0xf80a) uint8_t startup_coolant_temperature_raw;
static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf868) uint8_t purge_timer;
static volatile __xdata __at (0xf887) uint8_t coolant_axis_index;
static volatile __xdata __at (0xf888) uint8_t coolant_axis_fraction;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;

extern volatile __code const uint8_t cold_purge_temperature;
extern volatile __code const uint8_t hot_purge_duration;
extern volatile __code const uint8_t purge_interval;
extern volatile __code const uint8_t oxygen_enable_temperature;
extern volatile __code const uint8_t oxygen_disable_temperature;
extern volatile __code const uint8_t oxygen_warmup_temperature;
extern volatile __code const uint8_t oxygen_warmup_cold_seconds;
extern volatile __code const uint8_t oxygen_warmup_hot_seconds;
extern volatile __code const uint8_t warmed_engine_coolant_index;
extern __code const uint8_t purge_duty_raw[256];
extern __code const uint8_t catalyst_mixture_raw[256];
extern __code const uint8_t oxygen_ignition_raw[256];
extern volatile __code const uint8_t operating_zone_flags[256];
extern volatile __code const uint8_t base_mixture_by_coolant[39];
extern volatile __code const uint8_t base_mixture_temp_correction[39];

extern void interp_bilinear_u8(void);
extern void interp_curve_u8(void);
extern void limit_mixture_change_rate(void);
extern void blend_power_mode_mixture(void);
extern void update_oxygen_feedback(void);
extern void continue_fuel_mixture_checks(void);
extern void finish_catalyst_enrichment(void);

#define SAMPLE_RPM_CHARGE_TABLE(table)     \
    do {                                   \
        STOCK_DPTR = (uint16_t)(table);    \
        stock_r2 = rpm_q4;                 \
        stock_r3 = charge_q4;              \
        interp_bilinear_u8();              \
    } while (0)

#define SAMPLE_COOLANT_CURVE(table)        \
    do {                                   \
        stock_r2 = coolant_axis_index;     \
        stock_r3 = coolant_axis_fraction;  \
        STOCK_DPTR = (uint16_t)(table);    \
        interp_curve_u8();                 \
    } while (0)

void publish_evap_purge(void);
void select_oxygen_mixture(void);
void apply_open_loop_coolant_mix(void);

void continue_after_idle_airflow(void)
{
    if (evap_fitted && !(fuel_control_7b & 4) &&
        coolant_temperature_raw >= cold_purge_temperature) {
        if (one_second_tick && purge_timer != 0) {
            --purge_timer;
        }

        ACC = purge_timer;
        if (!(oxygen_sensor_configured && learning_sweep_active)) {
            if (purge_active) {
                if (ACC == 0) {
                    if (!(fuel_control_7b & 4)) {
                        purge_duty = 0;
                    }
                    purge_timer = purge_interval;
                    purge_active = 0;
                    fuel_mode_flag_5c = 0;
                } else if (!(fuel_control_7b & 4)) {
                    SAMPLE_RPM_CHARGE_TABLE(purge_duty_raw);
                    purge_duty = ACC;
                }
            } else if (ACC == 0) {
                purge_timer = hot_purge_duration;
                fuel_mode_flag_5c = 0;
                purge_active = 1;
            }
        }
    }
    publish_evap_purge();
}

void publish_evap_purge(void)
{
    if ((maf_adc_low || maf_adc_high || throttle_adc_low || throttle_adc_high) &&
        !(fuel_control_7b & 4)) {
        purge_duty = 0;
    }
    ACC = purge_duty;
    sfr_f3 = ACC;
    sfr_f2 = ACC;
    select_oxygen_mixture();
}

void select_oxygen_mixture(void)
{
    if (!oxygen_sensor_configured ||
        coolant_temperature_raw < oxygen_enable_temperature ||
        coolant_temperature_raw >= oxygen_disable_temperature) {
        goto rate_limit_only;
    }

    if (startup_coolant_temperature_raw < oxygen_warmup_temperature) {
        B = oxygen_warmup_cold_seconds;
    } else {
        B = oxygen_warmup_hot_seconds;
    }
    if (engine_run_seconds < B || !(oxygen_diagnostic_status & 1)) {
        goto rate_limit_only;
    }

    SAMPLE_RPM_CHARGE_TABLE(catalyst_mixture_raw);
    requested_mixture = ACC;
    if (!idle_regulating && fuel_cut_phase == 0 && !(ignition_and_airflow_control_7a & 4)) {
        SAMPLE_RPM_CHARGE_TABLE(oxygen_ignition_raw);
        requested_ignition = ACC;
    }

    if (!(operating_zone_flags[operating_point_index] & 4) ||
        oxygen_adc_low || oxygen_adc_high || oxygen_adaptation_fault_2c ||
        oxygen_adaptation_fault_2d || oxygen_heater_fault || (retained_status & 0x80)) {
        goto rate_limit_only;
    }

    limit_mixture_change_rate();
    SAMPLE_RPM_CHARGE_TABLE(catalyst_mixture_raw);
    if (ACC != requested_mixture) {
        goto blend_power;
    }
    if (!idle_regulating) {
        blend_power_mode_mixture();
        if (at_or_above_power_boundary) {
            goto cool_open_loop_mixture;
        }
    }
    if (oxygen_control_7d & 2) {
        fuel_mode_flag_5c = 0;
    }
    update_oxygen_feedback();
    continue_fuel_mixture_checks();
    return;

rate_limit_only:
    limit_mixture_change_rate();
blend_power:
    if (!idle_regulating) {
        blend_power_mode_mixture();
    }
cool_open_loop_mixture:
    apply_open_loop_coolant_mix();
}

void apply_open_loop_coolant_mix(void)
{
    if (idle_regulating) {
        finish_catalyst_enrichment();
        return;
    }

    B = base_mixture_by_coolant[warmed_engine_coolant_index];
    const uint8_t mixture = requested_mixture;
    stock_r1 = mixture >= B ? mixture - B : 0;
    SAMPLE_COOLANT_CURVE(base_mixture_temp_correction);
    stock_r1 = ((uint16_t)ACC * stock_r1) >> 8;
    SAMPLE_COOLANT_CURVE(base_mixture_by_coolant);
    requested_mixture = ACC + stock_r1;
}
