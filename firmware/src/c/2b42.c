#include "stock_registers.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sbit __at (0x44) mdu_interrupted;
static __sfr __at (0xf0) B;

static volatile __data __at (0x53) uint8_t rpm_half;
static volatile __data __at (0x54) uint8_t rpm_axis_code;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __xdata __at (0xfa5e) uint8_t rpm_eighth_increase;
static volatile __xdata __at (0xf8af) uint8_t rpm_eighth_bin;

extern void update_rpm_axis(void);
void publish_crank_rpm_axis_code(void);

void scale_crank_rpm_result(void)
{
    if (stock_r23 != 0)
        stock_r01 = UINT16_MAX;

    const uint16_t quotient = stock_r01;
    rpm_half = quotient >= 511 ? UINT8_MAX : (uint16_t)(quotient + 1) >> 1;

    const uint16_t biased = quotient + 4;
    const uint16_t rounded = biased >> 3;
    uint8_t eighth = (uint8_t)rounded;
    uint8_t borrow = (biased >> 2) & 1;

    /* SUBB consumes the carry left by the final rotate, or by the overflow
     * check when the bias wraps. On saturation the final rotate is skipped.
     */
    if (biased < quotient) {
        eighth = UINT8_MAX;
        borrow = 1;
    } else if (rounded > UINT8_MAX) {
        eighth = UINT8_MAX;
        borrow = (biased >> 10) & 1;
    }

    const int16_t increase = (int16_t)eighth - rpm_eighth - borrow;
    rpm_eighth_increase = increase < 0 ? 0 : (uint8_t)increase;
    rpm_eighth = eighth;
    rpm_eighth_bin = rpm_eighth >= 31 ? 31 : rpm_eighth;

    publish_crank_rpm_axis_code();
}

void publish_crank_rpm_axis_code(void)
{
    const uint16_t quotient = stock_r01;
    stock_r01 = quotient + 3;
    if (quotient <= UINT16_MAX - 3) {
        do {
            mdu_interrupted = 0;
            mdu_0 = stock_r0;
            mdu_1 = stock_r1;
            mdu_4 = 6;
            mdu_5 = 0;
            __asm
                nop
                nop
                nop
                nop
            __endasm;

            stock_r2 = mdu_0;
            stock_r3 = mdu_1;
            ACC = mdu_4;
            ACC = mdu_5;
        } while (mdu_interrupted);
        if (stock_r3 == 0) {
            rpm_axis_code = stock_r2;
            goto axis_ready;
        }
    }

    rpm_axis_code = UINT8_MAX;
axis_ready:
    update_rpm_axis();
}
