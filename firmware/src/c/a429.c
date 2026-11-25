#include <stdint.h>

static __sfr __at (0xe0) ACC;
static __sbit __at (0xe7) ACC_SIGN;

/* Input and output use the accumulator, independent of SDCC's C return ABI. */
void abs_accumulator_i8(void)
{
    if (ACC_SIGN) {
        /* Negate modulo 256 without touching the input carry. */
        ACC ^= UINT8_MAX;
        ++ACC;
    }
}
