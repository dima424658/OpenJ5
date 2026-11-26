#include "stock_registers.h"

enum { EEPROM_SCRATCH_ADDRESS = 0x94 };

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CARRY;
static __sbit __at (0xaf) EA;
static __sbit __at (0x12) eeprom_io_failed;

static volatile __data __at (4) uint8_t stock_r4;

extern void read_eeprom_bytes(void);
extern void write_eeprom_bytes(void);

void wait_for_eeprom_write(void);

/* The low-level routines take the EEPROM offset in R4, the count in R1
 * and the address of the indirect RAM buffer in R0. Carry reports failure.
 */
#define PREPARE_SCRATCH(length)                                  \
    do {                                                         \
        stock_r1 = (length);                                     \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                       \
    } while (0)

#define READ_RECORD(offset, length)                              \
    do {                                                         \
        stock_r4 = (offset);                                     \
        PREPARE_SCRATCH(length);                                 \
        read_eeprom_bytes();                                     \
    } while (0)

#define COPY_RECORD_IN(address, length)                          \
    do {                                                         \
        PREPARE_SCRATCH(length);                                 \
        STOCK_DPTR = (address);                                  \
        do {                                                     \
            ACC = *(volatile __idata uint8_t *)stock_r0;         \
            *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;       \
            ++stock_r0;                                          \
            ++STOCK_DPTR;                                        \
        } while (--stock_r1);                                    \
    } while (0)

#define TERMINATE_RECORD()                                       \
    do {                                                         \
        ACC = 0;                                                 \
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;           \
    } while (0)

#define WRITE_RECORD(address, offset, length)                    \
    do {                                                         \
        PREPARE_SCRATCH(length);                                 \
        STOCK_DPTR = (address);                                  \
        do {                                                     \
            ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;       \
            *(volatile __idata uint8_t *)stock_r0 = ACC;         \
            ++STOCK_DPTR;                                        \
            ++stock_r0;                                          \
        } while (--stock_r1);                                    \
        stock_r4 = (offset);                                     \
        PREPARE_SCRATCH(length);                                 \
        EA = 0;                                                  \
        write_eeprom_bytes();                                    \
        EA = 1;                                                  \
    } while (0)

void read_eeprom_record_f485(void)
{
    READ_RECORD(0x10, 7);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
    COPY_RECORD_IN(0xf485, 7);
    TERMINATE_RECORD();
}

void read_eeprom_record_f48d(void)
{
    READ_RECORD(0x18, 7);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
    COPY_RECORD_IN(0xf48d, 7);
    TERMINATE_RECORD();
}

void read_eeprom_record_f474(void)
{
    READ_RECORD(0x48, 7);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
    COPY_RECORD_IN(0xf474, 7);
    TERMINATE_RECORD();
}

void read_eeprom_record_f495(void)
{
    READ_RECORD(0x20, 8);
    if (CARRY) {
        goto read_failed;
    }
    COPY_RECORD_IN(0xf495, 8);

    READ_RECORD(0x28, 2);
    if (CARRY) {
        goto read_failed;
    }
    COPY_RECORD_IN(0xf49d, 2);
    TERMINATE_RECORD();
    return;

read_failed:
    eeprom_io_failed = 1;
}

void read_eeprom_record_f460(void)
{
    READ_RECORD(0x30, 8);
    if (CARRY) {
        goto read_failed;
    }
    COPY_RECORD_IN(0xf460, 8);

    READ_RECORD(0x38, 8);
    if (CARRY) {
        goto read_failed;
    }
    COPY_RECORD_IN(0xf468, 8);

    READ_RECORD(0x40, 3);
    if (CARRY) {
        goto read_failed;
    }
    COPY_RECORD_IN(0xf470, 3);
    TERMINATE_RECORD();
    return;

read_failed:
    eeprom_io_failed = 1;
}

void read_eeprom_record_f480(void)
{
    READ_RECORD(0xfb, 5);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
    COPY_RECORD_IN(0xf480, 5);
}

void write_eeprom_record_f485(void)
{
    WRITE_RECORD(0xf485, 0x10, 7);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
}

void write_eeprom_record_f48d(void)
{
    WRITE_RECORD(0xf48d, 0x18, 7);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
}

void write_eeprom_record_f474(void)
{
    WRITE_RECORD(0xf474, 0x48, 7);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
}

void write_eeprom_record_f495(void)
{
    WRITE_RECORD(0xf495, 0x20, 8);
    if (CARRY) {
        goto write_failed;
    }
    wait_for_eeprom_write();
    WRITE_RECORD(0xf49d, 0x28, 2);
    if (!CARRY) {
        return;
    }

write_failed:
    eeprom_io_failed = 1;
}

void write_eeprom_record_f460(void)
{
    WRITE_RECORD(0xf460, 0x30, 8);
    if (CARRY) {
        goto write_failed;
    }
    wait_for_eeprom_write();
    WRITE_RECORD(0xf468, 0x38, 8);
    if (CARRY) {
        goto write_failed;
    }
    wait_for_eeprom_write();
    WRITE_RECORD(0xf470, 0x40, 3);
    if (!CARRY) {
        return;
    }

write_failed:
    eeprom_io_failed = 1;
}

void wait_for_eeprom_write(void)
{
    stock_r3 = 0x34;
    stock_r4 = 0xff;
    do {
        do {
            --stock_r4;
        } while (stock_r4);
    } while (--stock_r3);
}
