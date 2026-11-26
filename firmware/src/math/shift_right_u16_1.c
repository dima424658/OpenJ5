#include "stock_registers.h"

/* Logical shifts of the stock word in R1:R0. CY receives the last bit out. */
void shift_right_u16_1(void)
{
    stock_r01 >>= 1;
}

void shift_right_u16_2(void)
{
    uint16_t value = stock_r01;
    uint8_t remaining = 2;
    do {
        value >>= 1;
    } while (--remaining != 0);
    stock_r01 = value;
}

void shift_right_u16_3(void)
{
    uint16_t value = stock_r01;
    uint8_t remaining = 3;
    do {
        value >>= 1;
    } while (--remaining != 0);
    stock_r01 = value;
}
