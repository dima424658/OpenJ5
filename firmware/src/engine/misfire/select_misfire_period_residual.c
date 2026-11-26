#include <stdbool.h>

#include "stock_registers.h"

static __sbit __at (0xd7) CY;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;

static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __xdata __at (0xf99f) uint16_t early_period_residual;
static volatile __xdata __at (0xf9a1) uint16_t late_period_residual;
static volatile __xdata __at (0xf9cf) uint16_t early_raw_period;
static volatile __xdata __at (0xf9cb) uint16_t late_raw_period;
static volatile __xdata __at (0xfa0b) uint16_t selected_period_residual;
static volatile __xdata __at (0xfa0d) uint16_t selected_raw_period;
static volatile __xdata __at (0xfa0f) uint16_t selected_period_ratio;
static volatile __xdata __at (0xfa11) uint16_t corrected_raw_period;

extern void skip_misfire_period_update(void);
extern void add_u16_register_pairs(void);
extern void subtract_u16_register_pairs(void);
extern void shift_left_saturating_1(void);
extern void continue_misfire_period_update(void);

void normalize_misfire_period(void);

void select_misfire_period_residual(void)
{
    const bool late_negative = (stock_r1 & 0x80) != 0;
    const uint8_t early_high = (uint8_t)(early_period_residual >> 8);

    // The negative branch tests bit 8 of the early residual.
    if (late_negative ? !(early_high & 1) : (early_high & 0x80)) {
        stock_r01 = 0x8000;
        selected_period_ratio = stock_r01;
        skip_misfire_period_update();
        return;
    }

    stock_r23 = early_period_residual;
    subtract_u16_register_pairs();

    // Nonnegative ties select the early sample. Negative-branch ties select late.
    if (late_negative ? CY : !CY) {
        selected_period_residual = stock_r23;
        stock_r23 = early_raw_period;
    } else {
        stock_r01 = late_period_residual;
        selected_period_residual = stock_r01;
        stock_r23 = late_raw_period;
    }
    normalize_misfire_period();
}

void normalize_misfire_period(void)
{
    selected_raw_period = stock_r23;
    stock_r01 = selected_period_residual;
    add_u16_register_pairs();
    corrected_raw_period = stock_r01;

    stock_r01 = selected_raw_period;
    shift_left_saturating_1();
    // In bank 1 this physical write supplies the MDU divisor in R3:R2.
    bank_one_r23 = stock_r01;
    stock_r01 = corrected_raw_period;

    mdu_0 = 0;
    mdu_1 = 0;
    mdu_2 = stock_r0;
    mdu_3 = stock_r1;
    mdu_4 = stock_r2;
    mdu_5 = stock_r3;
    __asm
        nop
        nop
        nop
        nop
        nop
        nop
    __endasm;
    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    (void)mdu_2;
    (void)mdu_3;
    (void)mdu_4;
    (void)mdu_5;
    selected_period_ratio = stock_r01;
    continue_misfire_period_update();
}
