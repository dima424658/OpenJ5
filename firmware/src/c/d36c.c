#include "stock_registers.h"

static __sfr __at (0xa5) spark_compare_control;
static __sfr __at (0xa6) dwell_compare_control;
static __sfr __at (0xe8) injector_port;
static __sfr __at (0xf6) injector_compare_control;
static __sfr __at (0xf8) ignition_port;

static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x73) service_pump_output;
static __sbit __at (0x7f) serial_extended_length;
static __sbit __at (0xf9) coil_1_level;
static __sbit __at (0xfa) coil_2_level;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x2e) uint8_t service_output_phases;
static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __data __at (0x7c) uint8_t output_flags;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];
static volatile __xdata __at (0xf95d) uint8_t injector_test_delay;
static volatile __xdata __at (0xf95e) uint8_t coil_test_duration;
static volatile __xdata __at (0xf95f) uint8_t coil_test_period;

#define OUTPUT_TEST_VALUE() (serial_rx_buffer[6 + (serial_extended_length ? 1 : 0)])

void apply_output_control_01_to_04(void)
{
    const uint8_t channel = stock_r4;
    if (stock_r3 == 0) {
        output_override_flags &= 0xf7;
        output_flags &= 0xf0;
    }
    if (stock_r3 == 0 || stock_r3 == 1) {
        if (channel < 1 || channel > 4) {
            stock_r2 = 0x12;
        } else {
            const uint8_t injector = UINT8_C(1) << (channel - 1);
            stock_r2 = (injector_port & injector) == 0;
        }
        return;
    }
    if (stock_r3 != 6) {
        stock_r2 = 0x12;
        return;
    }

    if (!fuel_mode_flag_56) {
        output_override_flags |= 8;
        service_pump_output = 0;
    }
    if (channel < 1 || channel > 4) {
        stock_r2 = 0x12;
        return;
    }

    const uint8_t injector = UINT8_C(1) << (channel - 1);
    output_flags |= injector;
    const uint8_t compare_mask = channel == 1 ? 0x7e : (uint8_t)~injector;
    injector_compare_control &= compare_mask;

    if (!OUTPUT_TEST_VALUE()) {
        injector_port |= injector;
        stock_r2 = 0;
    } else if (fuel_mode_flag_56) {
        output_flags &= 0xf0;
        stock_r2 = 0x31;
    } else {
        service_output_phases &= (uint8_t)~(injector << 4);
        stock_r2 = 1;
        injector_test_delay = 100;
    }
}

void apply_output_control_05(void)
{
    if (stock_r3 == 0) {
        output_flags &= 0xef;
    }
    if (stock_r3 == 0 || stock_r3 == 1) {
        stock_r2 = !coil_1_level;
    } else if (stock_r3 == 6) {
        spark_compare_control &= 0xbf;
        dwell_compare_control &= 0xbf;
        output_flags |= 0x10;
        if (!OUTPUT_TEST_VALUE()) {
            ignition_port |= 2;
            stock_r2 = 0;
        } else {
            stock_r2 = 1;
            coil_test_duration = 200;
            coil_test_period = 6;
            ignition_port &= 0xfd;
        }
    } else {
        stock_r2 = 0x12;
    }
}

void apply_output_control_06(void)
{
    if (stock_r3 == 0) {
        output_flags &= 0xdf;
    }
    if (stock_r3 == 0 || stock_r3 == 1) {
        stock_r2 = !coil_2_level;
    } else if (stock_r3 == 6) {
        spark_compare_control &= 0xdf;
        dwell_compare_control &= 0xdf;
        output_flags |= 0x20;
        if (!OUTPUT_TEST_VALUE()) {
            ignition_port |= 4;
            stock_r2 = 0;
        } else {
            stock_r2 = 1;
            coil_test_duration = 200;
            coil_test_period = 6;
            ignition_port &= 0xfb;
        }
    } else {
        stock_r2 = 0x12;
    }
}
