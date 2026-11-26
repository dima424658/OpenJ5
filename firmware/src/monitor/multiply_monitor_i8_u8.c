#include "stock_registers.h"

static __sfr __at (0xf0) B;

/* A is signed and B is unsigned. The result is returned in B:A. */
void multiply_monitor_i8_u8(void)
{
    const uint8_t input = ACC;
    const uint8_t magnitude = (input & 0x80) ? (uint8_t)-input : input;
    uint16_t product = (uint16_t)magnitude * B;
    if (input & 0x80) {
        product = -product;
    }
    B = product >> 8;
    ACC = (uint8_t)product;
}
