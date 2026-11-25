#include "stock_registers.h"

static __sbit __at (0xd7) CY;

/* A holds the byte, R0 the lower bound and R1 the upper bound. */
void clamp_stock_u8(void)
{
    const uint8_t input = ACC;
    const uint8_t lower = stock_r0;
    if (input < lower) {
        ACC = lower;
        CY = 1;
        return;
    }
    const uint8_t upper = stock_r1;
    if (input >= upper) {
        ACC = upper;
        CY = 1;
        return;
    }
    ACC = input;
    CY = 0;
}

/* A negative input uses only the lower bound and always sets carry.
 * A nonnegative input uses only the upper bound.
 */
void limit_stock_i8_by_sign(void)
{
    const uint8_t input = ACC;
    if (input & 0x80) {
        const uint8_t lower = stock_r0;
        if (input < lower) {
            ACC = lower;
            CY = 1;
            return;
        }
        ACC = input;
        CY = 1;
        return;
    }
    const uint8_t upper = stock_r1;
    if (input >= upper) {
        ACC = upper;
        CY = 1;
        return;
    }
    ACC = input;
    CY = 0;
}
