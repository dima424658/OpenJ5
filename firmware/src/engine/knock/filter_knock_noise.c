#include "stock_registers.h"

static __sfr __at (0xf0) B;
static volatile __data __at (4) uint8_t stock_r4;

// A is the new sample, R3 the previous result, and R2:R1 the retained sum.
void filter_knock_noise(void)
{
    uint16_t accumulator = ((uint16_t)stock_r2 << 8) | stock_r1;
    accumulator += ACC;
    accumulator -= stock_r3;
    stock_r1 = (uint8_t)accumulator;
    stock_r2 = accumulator >> 8;

    // The rounding bit rotates in a byte. A zero shift counter runs 256 times.
    uint16_t rounded = accumulator + (1u << ((uint8_t)(B - 1) & 7));
    stock_r4 = B;
    do {
        rounded >>= 1;
    } while (--stock_r4 != 0);

    stock_r3 = (uint8_t)rounded;
    ACC = stock_r3;
}
