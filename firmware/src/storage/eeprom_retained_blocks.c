#include "stock_registers.h"

enum {
    EEPROM_SCRATCH_ADDRESS = 0x94,
    SHUTDOWN_RECORD_OFFSET = 0x02,
    SHUTDOWN_RECORD_LENGTH = 6,
    BLOCK_F4A0_LENGTH = 6,
    BLOCK_F4A6_LENGTH = 8,
};

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CARRY;
static __sbit __at (0x12) eeprom_io_failed;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6c) shutdown_option_6c;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (6) uint8_t stock_r6;
static volatile __data __at (7) uint8_t stock_r7;
static volatile __xdata __at (0xf47c) uint8_t shutdown_block_selector;
static volatile __xdata __at (0xf47d) uint8_t shutdown_options;
static volatile __xdata __at (0xf47e) uint8_t shutdown_history[2];
static volatile __xdata __at (0xf4ae) uint8_t retained_fault_history[2];

extern void read_eeprom_bytes(void);
extern void write_eeprom_bytes(void);
extern void wait_for_eeprom_write(void);

#define INDIRECT_BYTE(reg) (*(volatile __idata uint8_t *)(reg))
#define XDATA_BYTE() (*(volatile __xdata uint8_t *)STOCK_DPTR)

#define EEPROM_IO(operation, offset, length)                     \
    do {                                                         \
        stock_r4 = (offset);                                     \
        stock_r1 = (length);                                     \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                       \
        operation();                                             \
    } while (0)

#define COPY_SCRATCH_TO_XDATA(address, count)                    \
    do {                                                         \
        stock_r1 = (count);                                      \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                       \
        STOCK_DPTR = (address);                                  \
        do {                                                     \
            XDATA_BYTE() = INDIRECT_BYTE(stock_r0);              \
            ++stock_r0;                                          \
            ++STOCK_DPTR;                                        \
        } while (--stock_r1);                                    \
    } while (0)

#define COPY_XDATA_TO_SCRATCH(address, count)                    \
    do {                                                         \
        stock_r1 = (count);                                      \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                       \
        STOCK_DPTR = (address);                                  \
        do {                                                     \
            INDIRECT_BYTE(stock_r0) = XDATA_BYTE();              \
            ++STOCK_DPTR;                                        \
            ++stock_r0;                                          \
        } while (--stock_r1);                                    \
    } while (0)

#define READ_RECORD_TO_REGISTER(target)                          \
    do {                                                         \
        (target) = INDIRECT_BYTE(stock_r0);                      \
        ++stock_r0;                                              \
    } while (0)

#define WRITE_RECORD_FROM(value)                                 \
    do {                                                         \
        INDIRECT_BYTE(stock_r0) = (value);                       \
        ++stock_r0;                                              \
    } while (0)

void restore_eeprom_block_f4a0(void)
{
    if (stock_r7 & 0x08) {
        return;
    }
    EEPROM_IO(read_eeprom_bytes, 0x50 + 8 * (stock_r7 & 3), BLOCK_F4A0_LENGTH);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
    COPY_SCRATCH_TO_XDATA(0xf4a0, BLOCK_F4A0_LENGTH);
}

void write_eeprom_block_f4a0(void)
{
    COPY_XDATA_TO_SCRATCH(0xf4a0, BLOCK_F4A0_LENGTH);
    eeprom_io_failed = 0;
    stock_r7 &= 0xf0;
    EEPROM_IO(write_eeprom_bytes, 0x50, BLOCK_F4A0_LENGTH);
    if (CARRY) {
        wait_for_eeprom_write();
        ++stock_r7;
        EEPROM_IO(write_eeprom_bytes, 0x58, BLOCK_F4A0_LENGTH);
        if (CARRY) {
            wait_for_eeprom_write();
            ++stock_r7;
            EEPROM_IO(write_eeprom_bytes, 0x60, BLOCK_F4A0_LENGTH);
        }
    }

    stock_r7 = (stock_r7 & 0xfb) | (CARRY ? 0x04 : 0);
    wait_for_eeprom_write();
}

void verify_eeprom_block_f4a0(void)
{
    EEPROM_IO(read_eeprom_bytes, 0x50 + 8 * (stock_r7 & 3), BLOCK_F4A0_LENGTH);
    if (!CARRY) {
        stock_r0 = EEPROM_SCRATCH_ADDRESS;
        stock_r1 = 0x25;
        stock_r2 = BLOCK_F4A0_LENGTH;
        do {
            if (INDIRECT_BYTE(stock_r0) != INDIRECT_BYTE(stock_r1)) {
                CARRY = 1;
                break;
            }
            ++stock_r0;
            ++stock_r1;
        } while (--stock_r2);
    }
    stock_r7 = (stock_r7 & 0xf7) | (CARRY ? 0x08 : 0);
}

void read_eeprom_shutdown_record(void)
{
    EEPROM_IO(read_eeprom_bytes, SHUTDOWN_RECORD_OFFSET, SHUTDOWN_RECORD_LENGTH);
    stock_r7 = 0;
    if (CARRY) {
        eeprom_io_failed = 1;
    } else {
        stock_r0 = EEPROM_SCRATCH_ADDRESS;
        READ_RECORD_TO_REGISTER(stock_r1);
        READ_RECORD_TO_REGISTER(stock_r2);
        READ_RECORD_TO_REGISTER(stock_r3);
        READ_RECORD_TO_REGISTER(stock_r4);
        READ_RECORD_TO_REGISTER(stock_r5);
        stock_r6 = INDIRECT_BYTE(stock_r0);

        /* Prefer the first pair unless the other two copies agree on a byte.
         * Any disagreement requests a rewrite of all three copies.
         */
        if (stock_r1 != stock_r3) {
            stock_r7 = 1;
        }
        if (stock_r1 != stock_r5) {
            stock_r7 = 1;
        }
        if (stock_r3 == stock_r5) {
            stock_r1 = stock_r3;
        } else {
            stock_r7 = 1;
        }
        if (stock_r2 != stock_r4) {
            stock_r7 = 1;
        }
        if (stock_r2 != stock_r6) {
            stock_r7 = 1;
        }
        if (stock_r4 == stock_r6) {
            stock_r2 = stock_r4;
        } else {
            stock_r7 = 1;
        }
    }

    shutdown_block_selector = stock_r1;
    shutdown_options = stock_r2;
    stock_r6 = stock_r2;
    extra_state_enabled = !(stock_r2 & 0x80);
    shutdown_option_6c = !(stock_r2 & 0x40);
    if (stock_r7) {
        stock_r0 = EEPROM_SCRATCH_ADDRESS;
        stock_r3 = 3;
        do {
            WRITE_RECORD_FROM(stock_r1);
            WRITE_RECORD_FROM(stock_r2);
        } while (--stock_r3);

        EEPROM_IO(write_eeprom_bytes, SHUTDOWN_RECORD_OFFSET, SHUTDOWN_RECORD_LENGTH);
        if (CARRY) {
            eeprom_io_failed = 1;
        }
        wait_for_eeprom_write();
    }

    stock_r7 = shutdown_block_selector;
    shutdown_history[0] = retained_fault_history[0];
    shutdown_history[1] = retained_fault_history[1];
}

void write_eeprom_shutdown_record(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = 3;
    do {
        WRITE_RECORD_FROM(stock_r7);
        WRITE_RECORD_FROM(shutdown_options);
    } while (--stock_r1);

    EEPROM_IO(write_eeprom_bytes, SHUTDOWN_RECORD_OFFSET, SHUTDOWN_RECORD_LENGTH);
    wait_for_eeprom_write();
}

void restore_eeprom_block_f4a6(void)
{
    if (stock_r7 & 0x80) {
        return;
    }
    EEPROM_IO(read_eeprom_bytes, 0x70 + 8 * ((stock_r7 >> 4) & 3), BLOCK_F4A6_LENGTH);
    if (CARRY) {
        eeprom_io_failed = 1;
    }
    COPY_SCRATCH_TO_XDATA(0xf4a6, BLOCK_F4A6_LENGTH);
}

void write_eeprom_block_f4a6(void)
{
    COPY_XDATA_TO_SCRATCH(0xf4a6, BLOCK_F4A6_LENGTH);
    stock_r6 = 0;
    EEPROM_IO(write_eeprom_bytes, 0x70, BLOCK_F4A6_LENGTH);
    if (CARRY) {
        wait_for_eeprom_write();
        ++stock_r6;
        EEPROM_IO(write_eeprom_bytes, 0x78, BLOCK_F4A6_LENGTH);
    }

    stock_r7 = (stock_r7 & 0x0f) | (stock_r6 << 4) | (CARRY ? 0x40 : 0);
    wait_for_eeprom_write();
}

void verify_eeprom_block_f4a6(void)
{
    EEPROM_IO(read_eeprom_bytes, 0x70 + 8 * ((stock_r7 >> 4) & 3), BLOCK_F4A6_LENGTH);
    if (!CARRY) {
        stock_r0 = EEPROM_SCRATCH_ADDRESS;
        STOCK_DPTR = 0xf4a6;
        stock_r2 = BLOCK_F4A6_LENGTH;
        do {
            if (XDATA_BYTE() != INDIRECT_BYTE(stock_r0)) {
                CARRY = 1;
                break;
            }
            ++stock_r0;
            ++STOCK_DPTR;
        } while (--stock_r2);
    }
    stock_r7 = (stock_r7 & 0x7f) | (CARRY ? 0x80 : 0);
}
