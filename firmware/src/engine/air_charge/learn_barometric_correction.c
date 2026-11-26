#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x44) mdu_interrupted;
static __sbit __at (0xaf) interrupts_enabled;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __xdata __at (0xf82a) uint8_t barometric_sample_count;
static volatile __xdata __at (0xf82c) uint16_t measured_air_charge;
static volatile __xdata __at (0xf88a) uint8_t barometric_sample_min;
static volatile __xdata __at (0xf88b) uint8_t barometric_sample_max;
static volatile __xdata __at (0xf88d) uint8_t previous_barometric_zone;
static volatile __xdata __at (0xf896) uint16_t barometric_sample_sum;

extern __code const uint8_t reverse_flow_coolant_gain[39];
extern __code const uint8_t barometric_max_sample_spread;
extern __code const uint8_t barometric_filter_length;

extern void shift_left_saturating_2(void);
extern void use_uncorrected_air_charge(void);
extern void use_air_charge_without_limit(void);
extern void average_baro_zone_samples(void);

void measure_barometric_sample(void);
void accumulate_barometric_sample(void);
void advance_baro_sample_window(void);

void learn_barometric_correction(void)
{
    stock_r4 = ACC;
    if (previous_barometric_zone != stock_r4) {
        previous_barometric_zone = stock_r4;
        use_uncorrected_air_charge();
        return;
    }
    measure_barometric_sample();
}

void measure_barometric_sample(void)
{
    B = stock_r3;
    interrupts_enabled = 0;
    stock_r01 = measured_air_charge;
    interrupts_enabled = 1;
    shift_left_saturating_2();

    do {
        const uint16_t denominator = (uint16_t)reverse_flow_coolant_gain[coolant_nearest_index] * B;
        ACC = (uint8_t)denominator;
        B = denominator >> 8;
        mdu_interrupted = 0;
        mdu_0 = 0;
        mdu_1 = 0;
        mdu_2 = stock_r0;
        mdu_3 = stock_r1;
        mdu_4 = ACC;
        mdu_5 = B;
        __asm
            nop
            nop
            nop
            nop
            nop
            nop
        __endasm;

        ACC = mdu_0;
        stock_r2 = mdu_1;
        stock_r3 = mdu_2;
        stock_r3 = mdu_3;
        stock_r3 = mdu_4;
        stock_r3 = mdu_5;
    } while (mdu_interrupted);

    if (ACC_SIGN)
        ++stock_r2;
    accumulate_barometric_sample();
}

void accumulate_barometric_sample(void)
{
    stock_r01 = barometric_sample_sum;
    stock_r01 += stock_r2;
    barometric_sample_sum = stock_r01;

    if (barometric_sample_count == 0) {
        barometric_sample_min = stock_r2;
        barometric_sample_max = stock_r2;
    } else {
        if (barometric_sample_min >= stock_r2)
            barometric_sample_min = stock_r2;
        else if (barometric_sample_max < stock_r2)
            barometric_sample_max = stock_r2;

        B = barometric_sample_min;
        B = (uint8_t)(barometric_sample_max - B);
        if (barometric_max_sample_spread < B) {
            use_uncorrected_air_charge();
            return;
        }
    }
    advance_baro_sample_window();
}

void advance_baro_sample_window(void)
{
    B = barometric_filter_length;
    ACC = barometric_sample_count + 1;
    barometric_sample_count = ACC;
    if (ACC != B) {
        use_air_charge_without_limit();
        return;
    }
    average_baro_zone_samples();
}
