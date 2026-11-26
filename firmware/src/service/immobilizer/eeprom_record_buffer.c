#include "eeprom_triplet.h"

enum { EEPROM_RECORD_LENGTH = 4 };

static __sfr16 __at (0x8382) STOCK_DPTR;

#define SCRATCH_BYTE (*(volatile __idata uint8_t *)stock_r0)
#define XDATA_BYTE (*(volatile __xdata uint8_t *)STOCK_DPTR)

void copy_eeprom_record_to_xdata(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = EEPROM_RECORD_LENGTH;
    do {
        ACC = SCRATCH_BYTE;
        XDATA_BYTE = ACC;
        ++stock_r0;
        ++STOCK_DPTR;
    } while (--stock_r1);
}

void copy_xdata_to_eeprom_buffer(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = EEPROM_RECORD_LENGTH;
    do {
        ACC = XDATA_BYTE;
        SCRATCH_BYTE = ACC;
        ++stock_r0;
        ++STOCK_DPTR;
    } while (--stock_r1);
}

/* This stock comparison keeps both pointers fixed. A match takes R2
 * iterations (256 when R2 starts at zero), and a mismatch leaves R2 intact.
 */
void compare_eeprom_record_byte(void)
{
    CARRY = 0;
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    do {
        ACC = XDATA_BYTE ^ SCRATCH_BYTE;
        if (ACC != 0) {
            CARRY = 1;
            return;
        }
    } while (--stock_r2);
}
