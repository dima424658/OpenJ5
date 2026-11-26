#include "stock_registers.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sbit __at (0x44) mdu_interrupted;

extern void sample_crank_period(void);
extern void scale_crank_rpm_result(void);

void calculate_rpm_from_period(void)
{
    mdu_interrupted = 0;
    /* Dividend 1,000,000 and the captured period in R3:R2. */
    mdu_0 = 0x40;
    mdu_1 = 0x42;
    mdu_2 = 0x0f;
    mdu_3 = 0;
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
    stock_r2 = mdu_2;
    stock_r3 = mdu_3;
    ACC = mdu_4;
    ACC = mdu_5;
    if (mdu_interrupted) {
        sample_crank_period();
        return;
    }
    scale_crank_rpm_result();
}
