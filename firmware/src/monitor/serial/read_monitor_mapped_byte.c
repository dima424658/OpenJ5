#include "stock_registers.h"

static __sfr __at (0x83) DPH;
static __sfr16 __at (0x8382) STOCK_DPTR;

enum { MONITOR_XDATA_PAGE = 0xf4 };

/* DPTR addresses the byte, R1 forces an XDATA read when nonzero.
 * ACC returns the byte and CY reports whether the address is below F400.
 */
void read_monitor_mapped_byte(void)
{
    if (DPH < MONITOR_XDATA_PAGE && stock_r1 == 0) {
        ACC = *(const __code uint8_t *)STOCK_DPTR;
        return;
    }
    ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
}
