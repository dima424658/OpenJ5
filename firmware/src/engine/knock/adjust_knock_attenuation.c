#include "stock_registers.h"

enum {
    ATTENUATION_STEP = 8,
    MAX_ATTENUATION = 16,
    ATTENUATION_DIAGNOSTIC_MASK = 0x1f,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x1f) knock_noise_low;
static __sbit __at (0x27) knock_noise_high;
static volatile __data __at (0x47) uint8_t knock_attenuation;
static volatile __xdata __at (0xf934) uint16_t knock_noise_accumulator;
static volatile __xdata __at (0xf936) uint8_t knock_average_noise;
static volatile __xdata __at (0xffce) uint8_t retained_knock_noise;

extern volatile __code const uint8_t attenuation_increase_noise;
extern volatile __code const uint8_t attenuation_decrease_noise;
extern volatile __code const uint8_t knock_low_noise_set;
extern volatile __code const uint8_t knock_low_noise_clear;
extern volatile __code const uint8_t knock_high_noise_clear;
extern volatile __code const uint8_t knock_high_noise_set;

void adjust_knock_attenuation(void)
{
    stock_r0 = knock_attenuation;
    stock_r23 = knock_noise_accumulator;
    B = retained_knock_noise;
    if (B > attenuation_decrease_noise) {
        if (stock_r0 == MAX_ATTENUATION) {
            return;
        }
        knock_attenuation = stock_r0 + ATTENUATION_STEP;
        ACC = B >> 1;
        retained_knock_noise = ACC;
        knock_average_noise = ACC;
        stock_r23 >>= 1;
        knock_noise_accumulator = stock_r23;
    } else if (B <= attenuation_increase_noise) {
        if (stock_r0 < ATTENUATION_STEP) {
            return;
        }
        knock_attenuation = stock_r0 - ATTENUATION_STEP;
        ACC = B + B;
        retained_knock_noise = ACC;
        knock_average_noise = ACC;
        stock_r23 = stock_r23 > UINT16_MAX / 2 ? UINT16_MAX : stock_r23 + stock_r23;
        knock_noise_accumulator = stock_r23;
    }
}

void diagnose_knock_noise_range(void)
{
    ACC = knock_attenuation & ATTENUATION_DIAGNOSTIC_MASK;
    if (ACC == 0) {
        if (knock_noise_low) {
            B = knock_low_noise_clear;
        } else {
            B = knock_low_noise_set;
        }
        if (retained_knock_noise < B) {
            knock_noise_low = 1;
        } else {
            knock_noise_low = 0;
        }
    } else if (ACC == MAX_ATTENUATION) {
        if (knock_noise_high) {
            B = knock_high_noise_clear;
        } else {
            B = knock_high_noise_set;
        }
        if (retained_knock_noise >= B) {
            knock_noise_high = 1;
        } else {
            knock_noise_high = 0;
        }
    }
}
