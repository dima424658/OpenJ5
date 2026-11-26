#include "stock_registers.h"

enum {
    EEPROM_SCRATCH_ADDRESS = 0x94,
    COPY_COUNT = 3,
    HISTORY_SIZE = 8,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x12) eeprom_io_failed;
static __sbit __at (0xd7) CY;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (7) uint8_t stock_r7;

static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xfa2b) uint8_t primary_period_dev[2];
static volatile __xdata __at (0xfa2d) uint8_t secondary_period_dev[2];

extern void write_eeprom_page1_bytes(void);
extern void wait_for_eeprom_write(void);

#define SCRATCH_BYTE() (*(volatile __idata uint8_t *)stock_r0)
#define WRITE_PAGE1(offset, length)                             \
    do {                                                        \
        stock_r4 = (offset);                                    \
        stock_r1 = (length);                                    \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                      \
        write_eeprom_page1_bytes();                             \
        if (CY) {                                               \
            eeprom_io_failed = 1;                               \
        }                                                       \
    } while (0)

void write_eeprom_diagnostic_state(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = COPY_COUNT;
    ACC = misfire_status;
    do {
        SCRATCH_BYTE() = ACC;
        ++stock_r0;
    } while (--stock_r1);

    WRITE_PAGE1(0x1c, COPY_COUNT);
    wait_for_eeprom_write();
}

void write_eeprom_shutdown_words(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r2 = primary_period_dev[0];
    stock_r3 = primary_period_dev[1];
    B = 0;

    for (;;) {
        stock_r1 = COPY_COUNT;
        do {
            SCRATCH_BYTE() = stock_r2;
            ++stock_r0;
            SCRATCH_BYTE() = stock_r3;
            ++stock_r0;
        } while (--stock_r1);
        if (B) {
            break;
        }

        stock_r2 = secondary_period_dev[0];
        stock_r3 = secondary_period_dev[1];
        B = 1;
    }

    WRITE_PAGE1(0x20, COPY_COUNT * 4);
    wait_for_eeprom_write();
}

void write_eeprom_shutdown_block(void)
{
    stock_r7 = 0;
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = HISTORY_SIZE;
    eeprom_io_failed = 0;
    do {
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
        SCRATCH_BYTE() = ACC;
        stock_r7 += ACC;
        ++STOCK_DPTR;
        ++stock_r0;
    } while (--stock_r1);

    WRITE_PAGE1(0x00, HISTORY_SIZE);
    wait_for_eeprom_write();
    WRITE_PAGE1(0x08, HISTORY_SIZE);
    wait_for_eeprom_write();
    WRITE_PAGE1(0x10, HISTORY_SIZE);

    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = COPY_COUNT;
    do {
        SCRATCH_BYTE() = stock_r7;
        ++stock_r0;
    } while (--stock_r1);
    wait_for_eeprom_write();

    WRITE_PAGE1(0x18, COPY_COUNT);
    wait_for_eeprom_write();
}
