#include "stock_registers.h"

enum {
    HISTORY_SIZE = 8,
    COPY_COUNT = 3,
    EEPROM_SCRATCH_ADDRESS = 0x94,
};

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CARRY;
static __sbit __at (0x12) eeprom_io_failed;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (6) uint8_t stock_r6;
static volatile __data __at (7) uint8_t stock_r7;
static volatile __data __at (0x51) uint8_t recovery_pass;
static volatile __data __at (0x7e) uint8_t mismatch_bits;
static volatile __xdata __at (0xf4ae) uint8_t fault_history[HISTORY_SIZE];
static volatile __xdata __at (0xfa43) uint8_t misfire_history[HISTORY_SIZE];

extern void read_eeprom_bytes(void);
extern void read_eeprom_page1_bytes(void);
extern void write_eeprom_bytes(void);
extern void wait_for_eeprom_write(void);
extern void write_eeprom_shutdown_block(void);

void compare_history_with_scratch(void);
void write_startup_fault_history(void);

#define INDIRECT_BYTE() (*(volatile __idata uint8_t *)stock_r0)
#define XDATA_BYTE() (*(volatile __xdata uint8_t *)STOCK_DPTR)

#define EEPROM_IO(operation, offset, length)                                  \
    do {                                                                      \
        stock_r4 = (uint8_t)(offset);                                         \
        stock_r1 = (length);                                                  \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                                    \
        operation();                                                          \
    } while (0)

#define COPY_HISTORY_FROM_SCRATCH()                                           \
    do {                                                                      \
        do {                                                                  \
            XDATA_BYTE() = INDIRECT_BYTE();                                   \
            ++stock_r0;                                                       \
            ++STOCK_DPTR;                                                     \
        } while (--stock_r1);                                                 \
    } while (0)

/* The first successful read seeds the buffer. Each following successful
 * read contributes one comparison bit, with one indicating a mismatch.
 */
#define READ_HISTORY_COPIES(read, offset, destination)                        \
    do {                                                                      \
        mismatch_bits = 0;                                                    \
        recovery_pass = 0;                                                    \
        stock_r5 = 0;                                                         \
        stock_r7 = 0;                                                         \
        stock_r6 = COPY_COUNT;                                                \
        do {                                                                  \
            EEPROM_IO(read, (offset) + HISTORY_SIZE * stock_r7, HISTORY_SIZE);\
            if (CARRY) {                                                      \
                eeprom_io_failed = 1;                                         \
            } else if (stock_r5 == 0) {                                       \
                stock_r1 = HISTORY_SIZE;                                      \
                STOCK_DPTR = (uint16_t)(destination);                         \
                stock_r0 = EEPROM_SCRATCH_ADDRESS;                            \
                COPY_HISTORY_FROM_SCRATCH();                                  \
                stock_r5 = 1;                                                 \
            } else {                                                          \
                stock_r1 = HISTORY_SIZE;                                      \
                STOCK_DPTR = (uint16_t)(destination);                         \
                compare_history_with_scratch();                               \
                mismatch_bits = (uint8_t)((mismatch_bits << 1) | CARRY);      \
            }                                                                 \
            ++stock_r7;                                                       \
        } while (--stock_r6);                                                 \
    } while (0)

#define PREPARE_RECOVERY_WRITE()                                              \
    do {                                                                      \
        mismatch_bits = 0;                                                    \
        recovery_pass = 1;                                                    \
        stock_r5 = 0;                                                         \
        stock_r7 = 1;                                                         \
        stock_r6 = 2;                                                         \
    } while (0)

#define WRITE_FAULT_COPY(offset, length)                                      \
    do {                                                                      \
        EEPROM_IO(write_eeprom_bytes, offset, length);                        \
        if (CARRY) {                                                          \
            eeprom_io_failed = 1;                                             \
        }                                                                     \
    } while (0)

void recover_startup_fault_history(void)
{
    READ_HISTORY_COPIES(read_eeprom_bytes, 0x80, fault_history);

    if (stock_r5 == 0) {
        return;
    }
    if (mismatch_bits == 3) {
        PREPARE_RECOVERY_WRITE();
    } else if ((mismatch_bits & 3) == 1 && recovery_pass) {
        eeprom_io_failed = 1;
        return;
    }
    STOCK_DPTR = (uint16_t)fault_history;
    write_startup_fault_history();
}

void recover_startup_misfire_history(void)
{
    READ_HISTORY_COPIES(read_eeprom_page1_bytes, 0, misfire_history);

    if (stock_r5 == 0) {
        return;
    }
    if (mismatch_bits == 3) {
        PREPARE_RECOVERY_WRITE();
    } else if ((mismatch_bits & 3) == 1 && recovery_pass) {
        eeprom_io_failed = 1;
        misfire_history[0] = 0;
        misfire_history[2] = 0;
        misfire_history[4] = 0;
        misfire_history[6] = 0;
        misfire_history[1] = 0x80;
        misfire_history[3] = 0x80;
        misfire_history[5] = 0x80;
        misfire_history[7] = 0x80;
    }
    STOCK_DPTR = (uint16_t)misfire_history;
    write_eeprom_shutdown_block();
}

void compare_history_with_scratch(void)
{
    CARRY = 0;
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    do {
        if (XDATA_BYTE() != INDIRECT_BYTE()) {
            CARRY = 1;
            break;
        }
        ++stock_r0;
        ++STOCK_DPTR;
    } while (--stock_r1);
}

void write_startup_fault_history(void)
{
    stock_r7 = 0;
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = HISTORY_SIZE;
    eeprom_io_failed = 0;
    do {
        ACC = XDATA_BYTE();
        INDIRECT_BYTE() = ACC;
        stock_r7 += ACC;
        ++STOCK_DPTR;
        ++stock_r0;
    } while (--stock_r1);

    WRITE_FAULT_COPY(0x80, HISTORY_SIZE);
    wait_for_eeprom_write();
    WRITE_FAULT_COPY(0x88, HISTORY_SIZE);
    wait_for_eeprom_write();
    WRITE_FAULT_COPY(0x90, HISTORY_SIZE);

    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = COPY_COUNT;
    do {
        INDIRECT_BYTE() = stock_r7;
        ++stock_r0;
    } while (--stock_r1);
    wait_for_eeprom_write();

    WRITE_FAULT_COPY(0x98, COPY_COUNT);
    wait_for_eeprom_write();
}

void copy_fault_history_from_scratch(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    STOCK_DPTR = (uint16_t)fault_history;
    stock_r1 = HISTORY_SIZE;
    COPY_HISTORY_FROM_SCRATCH();
}
