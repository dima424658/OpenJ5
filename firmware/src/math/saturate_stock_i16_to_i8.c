#include "stock_registers.h"

/* Clamp the signed word in R1:R0 to the signed byte range, returning A.
 * A representable word has a high byte that sign-extends its low byte.
 */
void saturate_stock_i16_to_i8(void)
{
    ACC = stock_r1;
    if (!ACC_SIGN) {
        if (ACC == 0) {
            ACC = stock_r0;
            if (!ACC_SIGN) {
                return;
            }
        }
        ACC = INT8_MAX;
        return;
    }

    ACC ^= UINT8_MAX;
    if (ACC == 0) {
        ACC = stock_r0;
        if (ACC_SIGN) {
            return;
        }
    }
    ACC = (uint8_t)INT8_MIN;
}
