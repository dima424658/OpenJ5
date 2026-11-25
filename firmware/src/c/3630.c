#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0x1c) oxygen_adc_low;
static __sbit __at (0x24) oxygen_adc_high;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x56) oxygen_test_condition_56;
static __sbit __at (0x5b) deceleration_fuel_cut;
static __sbit __at (0x5c) oxygen_test_condition_5c;

static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf8c3) uint8_t oxygen_fault_seconds;

extern __code const uint8_t oxygen_diagnostic_limits[2];
extern __code const uint8_t oxygen_fault_delay;
extern __code const uint8_t oxygen_low_alternate_delay;
extern __code const uint8_t oxygen_high_alternate_delay;

extern void continue_without_oxygen(void);
extern void update_oxygen_monitor_state(void);
void check_oxygen_high(void);
void clear_oxygen_fault_state(void);

void check_oxygen_diagnostics(void)
{
    if (deceleration_fuel_cut || oxygen_diagnostic_limits[0] < stock_r0)
        goto check_high;
    if (oxygen_adc_low || !tick_1s)
        goto done;

    ACC = oxygen_diagnostic_status;
    if (ACC_BIT2 || ACC_BIT1) {
        if (!oxygen_test_condition_5c)
            goto check_high;
        B = oxygen_fault_seconds;
        oxygen_fault_seconds = B + 1;
        if (oxygen_fault_delay >= B)
            goto done;
    } else {
        if (!oxygen_test_condition_56)
            goto done;
        B = oxygen_fault_seconds;
        oxygen_fault_seconds = B + 1;
        if (oxygen_low_alternate_delay >= B)
            goto done;
    }

    oxygen_adc_low = 1;
    oxygen_diagnostic_status &= 0xfe;
done:
    continue_without_oxygen();
    return;

check_high:
    check_oxygen_high();
}

void check_oxygen_high(void)
{
    if (oxygen_diagnostic_limits[1] >= stock_r0)
        goto clear_fault;
    if (oxygen_adc_high || !tick_1s)
        goto done;

    ACC = oxygen_diagnostic_status;
    if (ACC_BIT2 || ACC_BIT1) {
        if (!oxygen_test_condition_5c)
            goto clear_fault;
        B = oxygen_fault_seconds;
        oxygen_fault_seconds = B + 1;
        if (oxygen_fault_delay >= B)
            goto done;
    } else {
        if (!oxygen_test_condition_56)
            goto done;
        B = oxygen_fault_seconds;
        oxygen_fault_seconds = B + 1;
        if (oxygen_high_alternate_delay >= B)
            goto done;
    }

    oxygen_adc_high = 1;
    oxygen_diagnostic_status &= 0xfe;
done:
    continue_without_oxygen();
    return;

clear_fault:
    clear_oxygen_fault_state();
}

void clear_oxygen_fault_state(void)
{
    oxygen_adc_low = 0;
    oxygen_adc_high = 0;
    oxygen_fault_seconds = 0;
    update_oxygen_monitor_state();
}
