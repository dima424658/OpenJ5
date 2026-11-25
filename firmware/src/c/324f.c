#include "stock_registers.h"

enum { IGNITION_SUPPLY_ADC_CHANNEL = 1 };

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x4c) ignition_supply_low;
static volatile __data __at (0x00) uint8_t bank_zero_r0;
static volatile __xdata __at (0xf80d) uint8_t ignition_supply_raw;
static volatile __xdata __at (0xf81b) uint16_t ignition_supply_sum;
static volatile __xdata __at (0xf87c) uint8_t shutdown_elapsed;
static volatile __xdata __at (0xfa78) uint8_t ignition_supply_low_count;

extern volatile __code const uint8_t shutdown_voltage_raw;
extern volatile __code const uint8_t ignition_supply_low_samples;

extern void read_adc_8bit(void);
extern void accumulate_u8_xdata_u16(void);
extern void continue_sampling_sensors(void);

void detect_low_ignition_supply(void);

void sample_ignition_supply(void)
{
    B = IGNITION_SUPPLY_ADC_CHANNEL;
    read_adc_8bit();
    ignition_supply_raw = ACC;
    stock_r0 = ACC;
    STOCK_DPTR = (uint16_t)&ignition_supply_sum;
    accumulate_u8_xdata_u16();

    detect_low_ignition_supply();
}

void detect_low_ignition_supply(void)
{
    /* Comparison uses direct IRAM:00 rather than the active bank's R0. */
    if (shutdown_voltage_raw < bank_zero_r0) {
        goto clear_low_supply;
    }
    if (ignition_supply_low) {
        goto next_sensor;
    }

    B = ignition_supply_low_samples;
    ACC = ignition_supply_low_count + 1;
    ignition_supply_low_count = ACC;
    if (ACC >= B) {
        ignition_supply_low = 1;
        shutdown_elapsed = 0;
    }
    goto next_sensor;

clear_low_supply:
    ignition_supply_low = 0;
    ignition_supply_low_count = 0;

next_sensor:
    continue_sampling_sensors();
}
