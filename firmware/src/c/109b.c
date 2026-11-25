#include <stdbool.h>

#include "stock_registers.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __xdata __at (0xfa0f) uint16_t selected_period_ratio;
static volatile __xdata __at (0xfa27) uint16_t period_ratio_deviation;
static volatile __xdata __at (0xfa29) uint16_t weighted_period_deviation;
static volatile __xdata __at (0xfa2b) uint16_t primary_period_dev;
static volatile __xdata __at (0xfa2d) uint16_t secondary_period_dev;
static volatile __xdata __at (0xfa2f) uint16_t primary_period_magnitude;
static volatile __xdata __at (0xfa31) uint16_t secondary_period_magnitude;
static volatile __xdata __at (0xfa33) uint16_t peak_period_magnitude;
static volatile __xdata __at (0xfa43) uint16_t bin1_period_ratio;

extern volatile __code const uint8_t period_deviation_weight[2];

extern void add_u16_register_pairs(void);
extern void subtract_u16_register_pairs(void);
extern void negate_u16_register_pair(void);
extern void shift_right_u16_1(void);
extern void check_period_deviation_limit(void);

void filter_primary_period_dev(void);
void filter_secondary_period_dev(void);
void compare_period_deviations(void);

#define LOAD_DEVIATION_WEIGHT(low, high)     \
    do {                                     \
        (low) = period_deviation_weight[0];  \
        (high) = period_deviation_weight[1]; \
    } while (0)

#define MULTIPLY_PERIOD_TERMS() \
    do {                        \
        mdu_0 = stock_r2;       \
        mdu_4 = stock_r0;       \
        mdu_1 = stock_r3;       \
        mdu_5 = stock_r1;       \
        __asm nop __endasm;     \
        __asm nop __endasm;     \
        __asm nop __endasm;     \
        __asm nop __endasm;     \
    } while (0)

#define SCALE_PREVIOUS_DEVIATION()                        \
    do {                                                  \
        const bool was_negative = (stock_r1 & 0x80) != 0; \
        if (was_negative) {                               \
            negate_u16_register_pair();                   \
        }                                                 \
        bank_one_r23 = stock_r01;                         \
        LOAD_DEVIATION_WEIGHT(stock_r0, stock_r1);        \
        negate_u16_register_pair();                       \
        MULTIPLY_PERIOD_TERMS();                          \
        B = mdu_0;                                        \
        (void)mdu_1;                                      \
        stock_r0 = mdu_2;                                 \
        stock_r1 = mdu_3;                                 \
        if (was_negative) {                               \
            negate_u16_register_pair();                   \
        }                                                 \
    } while (0)

void update_misfire_period_bin1(void)
{
    stock_r23 = bin1_period_ratio;
    stock_r01 = selected_period_ratio;
    subtract_u16_register_pairs();
    period_ratio_deviation = stock_r01;

    const bool negative = (stock_r1 & 0x80) != 0;
    if (negative) {
        negate_u16_register_pair();
    }
    LOAD_DEVIATION_WEIGHT(stock_r2, stock_r3);
    MULTIPLY_PERIOD_TERMS();

    // Round after removing the low byte, then shift the retained word once more.
    const uint8_t low = mdu_0;
    stock_r0 = mdu_1;
    stock_r1 = mdu_2;
    B = mdu_3;
    stock_r01 += low >= 128;
    shift_right_u16_1();
    if (negative) {
        negate_u16_register_pair();
    }
    filter_primary_period_dev();
}

void filter_primary_period_dev(void)
{
    weighted_period_deviation = stock_r01;

    stock_r01 = primary_period_dev;
    SCALE_PREVIOUS_DEVIATION();
    stock_r23 = weighted_period_deviation;
    add_u16_register_pairs();
    primary_period_dev = stock_r01;
    if (stock_r1 & 0x80) {
        negate_u16_register_pair();
    }
    primary_period_magnitude = stock_r01;
    filter_secondary_period_dev();
}

void filter_secondary_period_dev(void)
{
    stock_r01 = secondary_period_dev;
    SCALE_PREVIOUS_DEVIATION();
    stock_r23 = weighted_period_deviation;
    add_u16_register_pairs();
    secondary_period_dev = stock_r01;
    if (stock_r1 & 0x80) {
        negate_u16_register_pair();
    }
    secondary_period_magnitude = stock_r01;
    compare_period_deviations();
}

void compare_period_deviations(void)
{
    stock_r23 = primary_period_magnitude;
    subtract_u16_register_pairs();
    stock_r01 = CY ? primary_period_magnitude : secondary_period_magnitude;
    peak_period_magnitude = stock_r01;
    check_period_deviation_limit();
}
