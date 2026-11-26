#include "stock_registers.h"

/* Return the magnitude in R1:R0, with its high byte in A.
 * The magnitude of INT16_MIN is retained as the unsigned word 0x8000.
 */
void abs_stock_i16(void)
{
    ACC = stock_r1;
    if (ACC_SIGN) {
        stock_r01 = (uint16_t)-stock_r01;
        ACC = stock_r1;
    }
}
