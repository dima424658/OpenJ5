#include "stock_registers.h"

enum { OXYGEN_ADC_CHANNEL = 10, OXYGEN_FILTER_WEIGHT_Q8 = 128 };

static __sfr __at (0xf0) B;
static __sbit __at (0x00) oxygen_sensor_configured;
static __sbit __at (0x4f) oxygen_threshold_set_4f;
static __sbit __at (0x67) oxygen_signal_rich;
static volatile __xdata __at (0xf811) uint8_t oxygen_filtered;
static volatile __xdata __at (0xf815) uint8_t oxygen_scaled;

extern __code const uint8_t oxygen_alternate_thresholds[2];
extern __code const uint8_t oxygen_switch_thresholds[2];
extern void read_adc_8bit(void);
extern void check_oxygen_diagnostics(void);
extern void continue_without_oxygen(void);

void classify_oxygen_signal(void);

void sample_adc10(void)
{
    B = OXYGEN_ADC_CHANNEL;
    read_adc_8bit();
    B = ACC;

    const uint8_t previous = oxygen_filtered;
    const uint8_t difference = previous - B;
    if (previous >= B) {
        const uint16_t magnitude = (uint16_t)difference * OXYGEN_FILTER_WEIGHT_Q8;
        B = -(uint8_t)(magnitude >> 8);
    } else {
        const uint16_t magnitude = (uint16_t)(uint8_t)(-difference) * OXYGEN_FILTER_WEIGHT_Q8;
        B = (uint8_t)(magnitude >> 8);
    }

    ACC = oxygen_filtered + B;
    stock_r0 = ACC;
    oxygen_filtered = ACC;
    oxygen_scaled = (uint8_t)(ACC * 4);
    classify_oxygen_signal();
}

void classify_oxygen_signal(void)
{
    if (!oxygen_threshold_set_4f) {
        B = oxygen_alternate_thresholds[0];
        if (!oxygen_signal_rich)
            B = oxygen_alternate_thresholds[1];
    } else {
        B = oxygen_switch_thresholds[0];
        if (!oxygen_signal_rich)
            B = oxygen_switch_thresholds[1];
    }

    oxygen_signal_rich = stock_r0 >= B;
    if (oxygen_sensor_configured) {
        check_oxygen_diagnostics();
        return;
    }
    continue_without_oxygen();
}
