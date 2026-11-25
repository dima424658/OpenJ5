#include "stock_registers.h"

void saturate_stock_word(void);

/* Unsigned left shifts with saturation of the word in R1:R0. */
void shift_left_saturating_1(void)
{
    const uint32_t doubled = (uint32_t)stock_r01 << 1;
    if (doubled > UINT16_MAX) {
        saturate_stock_word();
        return;
    }
    stock_r01 = doubled;
}

void saturate_stock_word(void)
{
    stock_r01 = UINT16_MAX;
}

void shift_left_saturating_2(void)
{
    uint16_t value = stock_r01;
    uint8_t remaining = 2;
    do {
        if (value > UINT16_MAX / 2) {
            saturate_stock_word();
            return;
        }
        value <<= 1;
    } while (--remaining != 0);
    stock_r01 = value;
}

void shift_left_saturating_3(void)
{
    uint16_t value = stock_r01;
    uint8_t remaining = 3;
    do {
        if (value > UINT16_MAX / 2) {
            saturate_stock_word();
            return;
        }
        value <<= 1;
    } while (--remaining != 0);
    stock_r01 = value;
}

void shift_left_saturating_4(void)
{
    const uint16_t value = stock_r01;
    if (value > UINT16_MAX / 16) {
        saturate_stock_word();
        return;
    }
    stock_r01 = value << 4;
}
