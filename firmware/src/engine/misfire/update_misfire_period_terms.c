#include <stdbool.h>

#include "stock_registers.h"

enum {
    PERIOD_HISTORY_LENGTH = 7,
    PERIOD_BASE_LOW = 0x19,
};

#define PERIOD_PAGE UINT16_C(0xfa00)
#define READ_PERIOD_LAG(lag)                                \
    (*(volatile __xdata uint16_t *)(PERIOD_PAGE |           \
        (uint8_t)(PERIOD_BASE_LOW + 2 * (stock_r5 - (lag) + \
            (stock_r5 < (lag) ? PERIOD_HISTORY_LENGTH : 0)))))

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x59) idle_mode;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (6) uint16_t stock_r67;
static volatile __data __at (0x08) uint16_t bank_one_r01;
static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __data __at (0x0e) uint16_t bank_one_r67;

static volatile __xdata __at (0xf93d) uint8_t misfire_mode_flags;
static volatile __xdata __at (0xf9df) uint16_t weighted_roughness_sample;
static volatile __xdata __at (0xf9e1) uint16_t filtered_roughness;
static volatile __xdata __at (0xf9f1) uint16_t misfire_period_factor;
static volatile __xdata __at (0xf9fc) uint32_t roughness_sum;

extern volatile __code const uint8_t roughness_filter_weight;
extern volatile __code const uint8_t roughness_idle_filter_weight;
extern volatile __code const uint8_t roughness_sample_gain;

extern void combine_filtered_misfire_scores(void);
extern void add_u16_register_pairs(void);
extern void subtract_u16_register_pairs(void);
extern void shift_right_u16_1(void);
extern void multiply_stock_u16_u8(void);
extern void shift_left_saturating_2(void);
extern void check_misfire_fuel_cut(void);

void filter_crank_roughness(void);

#define MDU_MULTIPLY(input, factor)       \
    do {                                  \
        mdu_0 = (uint8_t)(input);         \
        mdu_4 = (uint8_t)(factor);        \
        mdu_1 = (uint8_t)((input) >> 8);  \
        mdu_5 = (uint8_t)((factor) >> 8); \
        __asm nop __endasm;               \
        __asm nop __endasm;               \
        __asm nop __endasm;               \
        __asm nop __endasm;               \
    } while (0)

#define MDU_LOW_WORD (((uint16_t)mdu_1 << 8) | mdu_0)
#define ROUGHNESS_WEIGHT (idle_mode ? roughness_idle_filter_weight : roughness_filter_weight)

void update_misfire_period_terms(void)
{
    combine_filtered_misfire_scores();

    // Form 3*p[n-3] + p[n-5]; the preceding stages supply p[n-3] in R7:R6.
    // Each weighted sum keeps the carry from its low word in the third byte.
    MDU_MULTIPLY(stock_r67, UINT16_C(3));
    stock_r67 = MDU_LOW_WORD;
    B = mdu_2;
    (void)mdu_3;
    stock_r23 = READ_PERIOD_LAG(5);
    bank_one_r01 = stock_r67;
    add_u16_register_pairs();
    stock_r4 = B + CY;
    bank_one_r67 = stock_r01;

    // Form 3*p[n-4] + p[n-2] and subtract it from the first sum.
    stock_r23 = READ_PERIOD_LAG(4);
    MDU_MULTIPLY(stock_r23, UINT16_C(3));
    stock_r23 = MDU_LOW_WORD;
    B = mdu_2;
    (void)mdu_3;
    stock_r01 = READ_PERIOD_LAG(2);
    add_u16_register_pairs();
    B += CY;

    stock_r23 = stock_r01;
    stock_r01 = bank_one_r67;
    subtract_u16_register_pairs();
    const bool negative = stock_r4 < (uint16_t)B + CY;
    stock_r4 -= B + CY;
    if (negative) {
        stock_r4 = (uint8_t)(-stock_r4 - (stock_r01 != 0));
        stock_r01 = -stock_r01;
    }
    if (stock_r4 != 0) {
        stock_r01 = UINT16_MAX;
    }
    bank_one_r23 = stock_r01;
    filter_crank_roughness();
}

void filter_crank_roughness(void)
{
    stock_r01 = misfire_period_factor;
    shift_right_u16_1();
    MDU_MULTIPLY(stock_r01, stock_r23);
    stock_r01 = MDU_LOW_WORD;
    if ((mdu_2 | mdu_3) != 0) {
        stock_r01 = UINT16_MAX;
    }

    B = ROUGHNESS_WEIGHT;
    multiply_stock_u16_u8();
    B = roughness_sample_gain;
    multiply_stock_u16_u8();
    shift_left_saturating_2();
    weighted_roughness_sample = stock_r01;

    B = -ROUGHNESS_WEIGHT;
    stock_r01 = filtered_roughness;
    multiply_stock_u16_u8();
    stock_r23 = weighted_roughness_sample;
    add_u16_register_pairs();
    B = CY;
    stock_r23 = B;
    roughness_sum = ((uint32_t)stock_r23 << 16) | stock_r01;
    if (B != 0) {
        stock_r01 = UINT16_MAX;
    }

    if (misfire_mode_flags & 0x80) {
        stock_r01 = 0;
    }
    filtered_roughness = stock_r01;
    check_misfire_fuel_cut();
}
