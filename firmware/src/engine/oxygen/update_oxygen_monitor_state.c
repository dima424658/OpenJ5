#include "stock_registers.h"

enum { OXYGEN_READY = 0x01, HEATER_WAIT_COMPLETE = 0x04 };

static __sfr __at (0xf0) B;
static __sbit __at (0xf0) B_BIT0;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0x2b) oxygen_heater_fault;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x4f) oxygen_threshold_set_4f;
static __sbit __at (0x53) oxygen_monitor_reset_53;
static __sbit __at (0x5c) oxygen_test_condition_5c;
static __sbit __at (0x62) inhibit_oxygen_diagnostics_62;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __xdata __at (0xf803) uint8_t oxygen_ready_count;
static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf8c4) uint8_t oxygen_inactive_count;
static volatile __xdata __at (0xf8c5) uint8_t oxygen_heater_count;

extern __code const uint8_t oxygen_ready_limits[3];
extern __code const uint8_t oxygen_heater_limits[2];
extern __code const uint8_t oxygen_learning_temperature;
extern void continue_sensor_diagnostics(void);

void check_oxygen_readiness(void);
void clear_oxygen_readiness(void);
void check_oxygen_inactivity(void);
void continue_without_oxygen(void);

void update_oxygen_monitor_state(void)
{
    ACC = oxygen_diagnostic_status;
    if (!ACC_BIT2 && ACC_BIT1 && tick_1s) {
        B = oxygen_heater_limits[1];
        ACC = oxygen_heater_count + 1;
        oxygen_heater_count = ACC;
        if (ACC >= B)
            oxygen_diagnostic_status |= HEATER_WAIT_COMPLETE;
    }

    if (oxygen_monitor_reset_53) {
        oxygen_diagnostic_status = 0;
        oxygen_heater_count = 0;
        continue_sensor_diagnostics();
        return;
    }
    check_oxygen_readiness();
}

void check_oxygen_readiness(void)
{
    CY = oxygen_ready_limits[0] < stock_r0;
    B_BIT0 = CY;
    CY = oxygen_ready_limits[1] < stock_r0;
    CY = !CY && B_BIT0;
    if (CY) {
        ACC = oxygen_diagnostic_status;
        if (ACC_BIT0) {
            check_oxygen_inactivity();
            return;
        }
        clear_oxygen_readiness();
        return;
    }

    oxygen_inactive_count = 0;
    B = oxygen_ready_limits[2];
    ACC = oxygen_ready_count + 1;
    if (ACC != 0)
        oxygen_ready_count = ACC;
    if (ACC >= B) {
        oxygen_heater_fault = 0;
        oxygen_diagnostic_status |= OXYGEN_READY;
    }
    continue_without_oxygen();
}

void clear_oxygen_readiness(void)
{
    oxygen_ready_count = 0;
    oxygen_diagnostic_status &= (uint8_t)~OXYGEN_READY;
    continue_without_oxygen();
}

void check_oxygen_inactivity(void)
{
    ACC = oxygen_diagnostic_status;
    if (!ACC_BIT2 || !oxygen_test_condition_5c || inhibit_oxygen_diagnostics_62 ||
            !oxygen_threshold_set_4f) {
        oxygen_inactive_count = 0;
    } else if (tick_1s) {
        B = oxygen_heater_limits[0];
        ACC = oxygen_inactive_count + 1;
        if (ACC != 0)
            oxygen_inactive_count = ACC;
        if (ACC >= B) {
            oxygen_heater_fault = 1;
            clear_oxygen_readiness();
            return;
        }
    }
    continue_without_oxygen();
}

void continue_without_oxygen(void)
{
    if (!inhibit_oxygen_diagnostics_62) {
        CY = oxygen_learning_temperature < coolant_temperature_raw;
        ACC = oxygen_diagnostic_status;
        if (CY && ACC_BIT2 && !ACC_BIT0)
            oxygen_heater_fault = 1;
    }
    continue_sensor_diagnostics();
}
