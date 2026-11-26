#include "stock_registers.h"

enum {
    NOISE_UPDATE_COUNT_LIMIT = 5,
    ATTENUATION_MASK = 0x03,
    ATTENUATION_SHIFT = 3,
    NOISE_BIN_SHIFT = 3,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x37) noise_adaptation_active;
static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x08) uint8_t bank_one_noise_floor;
static volatile __data __at (0x43) uint8_t knock_mask_index;
static volatile __data __at (0x47) uint8_t knock_attenuation;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4f) uint8_t knock_noise_threshold;
static volatile __data __at (0x71) uint8_t pending_knock_mask;
static volatile __xdata __at (0xf934) uint16_t knock_noise_accumulator;
static volatile __xdata __at (0xf936) uint8_t knock_average_noise;
static volatile __xdata __at (0xf937) uint8_t knock_absolute_threshold;
static volatile __xdata __at (0xf965) uint8_t knock_noise_hold_count;
static volatile __xdata __at (0xf9aa) uint8_t normalized_knock_noise;
static volatile __xdata __at (0xf9ab) uint8_t knock_noise_bin;
static volatile __xdata __at (0xffce) uint8_t retained_knock_noise;

extern volatile __code const uint8_t knock_noise_adaptation_temp;
extern volatile __code const uint8_t knock_noise_filter_coefficient;
extern __code const uint8_t knock_relative_threshold_q4[32];
extern volatile __code const uint8_t knock_event_masks[16];

extern void filter_knock_noise(void);
extern void adjust_knock_attenuation(void);
extern void diagnose_knock_noise_range(void);
extern void finish_knock_sample(void);

void adapt_knock_noise_reference(void);
void detect_knock_event(void);

void evaluate_knock_sample(void)
{
    if (!noise_adaptation_active) {
        detect_knock_event();
        return;
    }
    if (coolant_temperature_raw <= knock_noise_adaptation_temp ||
            knock_noise_hold_count >= NOISE_UPDATE_COUNT_LIMIT) {
        finish_knock_sample();
        return;
    }
    adapt_knock_noise_reference();
}

void adapt_knock_noise_reference(void)
{
    stock_r0 = stock_r4 >> 1;
    stock_r3 = knock_average_noise;
    // In register bank 1 the fixed IRAM operand is the R0 just written.
    if (stock_r3 < bank_one_noise_floor) {
        stock_r4 = (uint8_t)(stock_r3 << 1) | (stock_r3 >> 7);
    }

    const uint16_t accumulator = knock_noise_accumulator;
    stock_r1 = (uint8_t)accumulator;
    stock_r2 = accumulator >> 8;
    B = knock_noise_filter_coefficient;
    ACC = stock_r4;
    filter_knock_noise();
    knock_average_noise = ACC;
    retained_knock_noise = ACC;
    knock_noise_accumulator = ((uint16_t)stock_r2 << 8) | stock_r1;

    adjust_knock_attenuation();
    diagnose_knock_noise_range();

    const uint8_t attenuation = (knock_attenuation >> ATTENUATION_SHIFT) & ATTENUATION_MASK;
    B = attenuation == 1 ? 2 : attenuation == 0 ? 1 : 4;
    const uint16_t normalized = (uint16_t)retained_knock_noise * B;
    stock_r0 = normalized > UINT8_MAX ? UINT8_MAX : normalized;
    normalized_knock_noise = stock_r0;
    ACC = stock_r0 >> NOISE_BIN_SHIFT;
    knock_noise_bin = ACC;
    B = knock_relative_threshold_q4[ACC];

    const uint16_t relative = ((uint16_t)retained_knock_noise * B) >> 4;
    knock_noise_threshold = relative > UINT8_MAX ? UINT8_MAX : relative;
    finish_knock_sample();
}

void detect_knock_event(void)
{
    knock_noise_hold_count = 0;
    // R1 carries the sample, R3 the cylinder threshold. The final comparison
    // includes the low bit shifted out of the relative threshold as a borrow.
    if (stock_r1 >= knock_absolute_threshold ||
            (stock_r1 > stock_r3 &&
             (uint16_t)stock_r1 + (knock_noise_threshold & 1) > (knock_noise_threshold >> 1))) {
        stock_r1 = knock_mask_index;
        pending_knock_mask |= knock_event_masks[stock_r1];
    }
    finish_knock_sample();
}
