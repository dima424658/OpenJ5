#include "eeprom_triplet.h"

enum {
    EEPROM_RECORD_LENGTH = 4,
    EEPROM_RECORD_FIRST = 0x08,
    EEPROM_RECORD_SECOND = 0x0c,
    EEPROM_RECORD_THIRD = 0x2c,
};

static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;

void store_immobilizer_record_copies(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = EEPROM_RECORD_LENGTH;
    eeprom_io_failed = 0;
    do {
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
        *(volatile __idata uint8_t *)stock_r0 = ACC;
        ++STOCK_DPTR;
        ++stock_r0;
    } while (--stock_r1);

    EEPROM_IO(write_eeprom_bytes, EEPROM_RECORD_FIRST, EEPROM_RECORD_LENGTH);
    wait_for_eeprom_write();
    EEPROM_IO(write_eeprom_bytes, EEPROM_RECORD_SECOND, EEPROM_RECORD_LENGTH);
    wait_for_eeprom_write();
    EEPROM_IO(write_eeprom_bytes, EEPROM_RECORD_THIRD, EEPROM_RECORD_LENGTH);
    if (eeprom_io_failed) {
        immobilizer_status &= 0x7f;
    }
    wait_for_eeprom_write();
}

void store_immobilizer_status_copies(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    ACC = immobilizer_status;
    *(volatile __idata uint8_t *)stock_r0 = ACC;
    EEPROM_IO(write_eeprom_bytes, EEPROM_RECORD_FIRST + 3, 1);
    wait_for_eeprom_write();
    EEPROM_IO(write_eeprom_bytes, EEPROM_RECORD_SECOND + 3, 1);
    wait_for_eeprom_write();
    EEPROM_IO(write_eeprom_bytes, EEPROM_RECORD_THIRD + 3, 1);
    wait_for_eeprom_write();
}
