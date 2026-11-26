#include "stock_registers.h"

enum { MDU_SHIFT_LEFT_TWO = 0x22 };

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xef) mdu_control;

static volatile __xdata __at (0xfa00) uint16_t filtered_misfire_scores[4];
static volatile __xdata __at (0xfa7a) uint16_t misfire_period_correction;

// The stock ADDC doubles the high accumulator before adding each carry.
#define ADD_FILTERED_SCORE(index)                                               \
    do {                                                                        \
        const uint32_t sum = (uint32_t)stock_r01 + filtered_misfire_scores[index]; \
        stock_r01 = (uint16_t)sum;                                               \
        stock_r2 = 2 * stock_r2 + (sum >> 16);                                    \
    } while (0)

void combine_filtered_misfire_scores(void)
{
    stock_r01 = filtered_misfire_scores[0];
    stock_r2 = 0;
    ADD_FILTERED_SCORE(1);
    ADD_FILTERED_SCORE(2);
    ADD_FILTERED_SCORE(3);

    mdu_0 = stock_r0;
    mdu_1 = stock_r1;
    mdu_2 = stock_r2;
    mdu_3 = 0;
    mdu_control = MDU_SHIFT_LEFT_TWO;
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
    stock_r2 = mdu_2;
    stock_r3 = mdu_3;
    misfire_period_correction = stock_r01;
}
