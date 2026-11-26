#include "stock_registers.h"

enum { HOLD_IAC_TARGET = 0x20 };

static __sfr __at (0xf0) B;
static __sbit __at (0xf7) B_SIGN;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x6d) int8_t idle_rpm_error;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;
static volatile __xdata __at (0xf84c) int8_t previous_idle_rpm_error;

extern volatile __code const uint8_t idle_iac_cold_temp_limit;
extern void apply_idle_airflow_feedback(void);

void calculate_idle_rpm_delta(void);
void suppress_cold_iac_delta(void);

void prepare_idle_airflow_feedback(void)
{
    if (ignition_and_airflow_control_7a & HOLD_IAC_TARGET) {
        return;
    }
    calculate_idle_rpm_delta();
}

void calculate_idle_rpm_delta(void)
{
    B = (uint8_t)(idle_rpm_error - previous_idle_rpm_error);

    /* A difference between errors of the same sign always fits in int8_t. */
    if (previous_idle_rpm_error < 0 && idle_rpm_error < 0) {
        goto save_error;
    }
    if (previous_idle_rpm_error >= 0 && idle_rpm_error >= 0) {
        goto save_error;
    }
    if (idle_rpm_error >= 0) {
        if (B_SIGN) {
            B = INT8_MAX;
        }
    } else if (!B_SIGN) {
        B = (uint8_t)INT8_MIN;
    }

save_error:
    previous_idle_rpm_error = idle_rpm_error;
    suppress_cold_iac_delta();
}

void suppress_cold_iac_delta(void)
{
    if (coolant_temperature_raw <= idle_iac_cold_temp_limit
            && idle_rpm_error >= 0 && B_SIGN) {
        B = 0;
    }
    apply_idle_airflow_feedback();
}
