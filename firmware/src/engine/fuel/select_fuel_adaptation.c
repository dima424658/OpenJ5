#include "stock_registers.h"

enum {
    HOLD_INJECTION_CORRECTION = 0x10,
    RETAINED_ADAPTATION_INVALID = 0x80,
    INJECTION_CORRECTION_MIN = 0x0f,
    INJECTION_CORRECTION_MAX = 0xf0,
    INJECTION_CORRECTION_UNITY_Q7 = 0x80,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x00) oxygen_sensor_configured;
static __sbit __at (0x08) co_potentiometer_configured;
static __sbit __at (0x1c) oxygen_adc_low;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x24) oxygen_adc_high;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x26) maf_adc_high;
static __sbit __at (0x2b) oxygen_heater_fault;
static __sbit __at (0x2c) oxygen_adaptation_fault_2c;
static __sbit __at (0x2d) oxygen_adaptation_fault_2d;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x5c) fuel_mode_flag_5c;
static __sbit __at (0x5e) purge_table_selected;

static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __data __at (0x6e) uint8_t oxygen_control_adjustment;
static volatile __data __at (0x6f) uint8_t injection_time_correction;
static volatile __data __at (0x77) uint8_t idle_injection_correction;
static volatile __data __at (0x7a) uint8_t control_7a;
static volatile __xdata __at (0xf82b) uint8_t co_correction;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xfd00) uint8_t retained_injection_correction[256];
static volatile __xdata __at (0xfe00) uint8_t retained_purge_correction[256];

extern __code const uint8_t co_sensitivity_raw[256];
extern __code const uint8_t initial_injection_correction[256];
extern __code const uint8_t idle_initial_injection_map[256];
extern void interp_bilinear_u8(void);
extern void interp_bilinear_xdata(void);
extern void clamp_stock_u8(void);
extern void finish_ignition_corrections(void);

void select_initial_injection(void);
void interp_initial_injection(void);
void bound_injection_correction(void);
void store_injection_correction(void);

void select_fuel_adaptation(void)
{
    if (throttle_adc_low || throttle_adc_high || maf_adc_low || maf_adc_high ||
        oxygen_adc_low || oxygen_adc_high || oxygen_heater_fault ||
        oxygen_adaptation_fault_2c || oxygen_adaptation_fault_2d ||
        (retained_status & RETAINED_ADAPTATION_INVALID) || !oxygen_sensor_configured) {
        select_initial_injection();
        return;
    }

    if (purge_table_selected) {
        STOCK_DPTR = (uint16_t)retained_purge_correction;
    } else {
        STOCK_DPTR = (uint16_t)retained_injection_correction;
    }
    stock_r2 = rpm_q4;
    stock_r3 = charge_q4;
    interp_bilinear_xdata();
    if (fuel_mode_flag_5c) {
        ACC += oxygen_control_adjustment;
    }
    bound_injection_correction();
}

void select_initial_injection(void)
{
    if (!co_potentiometer_configured) {
        interp_initial_injection();
        return;
    }
    B = co_sensitivity_raw[operating_point_index];
    ACC = co_correction;
    // Truncate the signed product toward zero before adding it to the base.
    if (ACC_SIGN) {
        ACC = (uint8_t)-ACC;
        B = -((uint16_t)ACC * B >> 8);
    } else {
        B = (uint16_t)ACC * B >> 8;
    }

    ACC = initial_injection_correction[operating_point_index];
    if ((int8_t)B >= 0) {
        ACC += B;
        if (ACC < B) {
            ACC = INJECTION_CORRECTION_MAX;
            store_injection_correction();
            return;
        }
    } else {
        ACC += B;
        if (ACC >= B) {
            ACC = INJECTION_CORRECTION_MIN;
            store_injection_correction();
            return;
        }
    }
    bound_injection_correction();
}

void interp_initial_injection(void)
{
    STOCK_DPTR = (uint16_t)initial_injection_correction;
    stock_r2 = rpm_q4;
    stock_r3 = charge_q4;
    interp_bilinear_u8();
    bound_injection_correction();
}

void bound_injection_correction(void)
{
    stock_r0 = INJECTION_CORRECTION_MIN;
    stock_r1 = INJECTION_CORRECTION_MAX;
    clamp_stock_u8();
    store_injection_correction();
}

void store_injection_correction(void)
{
    B = ACC;
    if (!(control_7a & HOLD_INJECTION_CORRECTION)) {
        injection_time_correction = B;
    }

    if (idle_regulating) {
        STOCK_DPTR = (uint16_t)idle_initial_injection_map;
        stock_r2 = rpm_q4;
        stock_r3 = charge_q4;
        interp_bilinear_u8();
        stock_r0 = INJECTION_CORRECTION_MIN;
        stock_r1 = INJECTION_CORRECTION_MAX;
        clamp_stock_u8();
        idle_injection_correction = ACC;
    } else {
        idle_injection_correction = INJECTION_CORRECTION_UNITY_Q7;
    }
    finish_ignition_corrections();
}
