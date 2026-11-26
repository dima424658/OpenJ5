#include "stock_registers.h"

static __sfr __at (0xf0) B;

/* The service monitor embeds a byte-identical copy at EDC2. */
#ifdef MONITOR_MULTIPLY_COPY
#define MULTIPLY_U16_U8 multiply_monitor_u16_u8
#define MULTIPLY_I16_U8 multiply_monitor_i16_u8
#else
#define MULTIPLY_U16_U8 multiply_stock_u16_u8
#define MULTIPLY_I16_U8 multiply_stock_i16_u8
#endif

/* Multiply the word in R1:R0 by B. The product is returned in R1:R0:B,
 * with A also holding the high byte.
 */
void MULTIPLY_U16_U8(void)
{
    const uint8_t factor = B;
    const uint16_t high_product = (uint16_t)stock_r1 * factor;
    const uint16_t low_product = (uint16_t)stock_r0 * factor;

    B = (uint8_t)low_product;
    stock_r01 = high_product + (low_product >> 8);
    ACC = stock_r1;
}

/* The word is signed and the factor in B is unsigned. */
void MULTIPLY_I16_U8(void)
{
    const uint8_t factor = B;
    const uint8_t high = stock_r1;
    uint16_t high_product;
    if ((high & 0x80) == 0) {
        high_product = (uint16_t)high * factor;
    } else {
        /* Interpret the high byte as negative using modular arithmetic. */
        high_product = (uint16_t)high * factor - ((uint16_t)factor << 8);
    }
    const uint16_t low_product = (uint16_t)stock_r0 * factor;

    B = (uint8_t)low_product;
    stock_r01 = high_product + (low_product >> 8);
    ACC = stock_r1;
}
