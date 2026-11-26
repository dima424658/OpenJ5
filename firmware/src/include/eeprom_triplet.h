#ifndef JANVAR_EEPROM_TRIPLET_H
#define JANVAR_EEPROM_TRIPLET_H

#include "stock_registers.h"

enum { EEPROM_SCRATCH_ADDRESS = 0x94, TRIPLET_COPY_COUNT = 3 };

static __sbit __at (0xd7) CARRY;
static __sbit __at (0x12) eeprom_io_failed;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __idata __at (EEPROM_SCRATCH_ADDRESS) uint8_t scratch[TRIPLET_COPY_COUNT];

extern void read_eeprom_bytes(void);
extern void write_eeprom_bytes(void);
extern void wait_for_eeprom_write(void);

#define EEPROM_IO(operation, offset, length)                             \
    do {                                                                 \
        stock_r4 = (uint8_t)(offset);                                             \
        stock_r1 = (length);                                             \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;                               \
        operation();                                                     \
        if (CARRY) {                                                     \
            eeprom_io_failed = 1;                                        \
        }                                                                \
    } while (0)

#define REPAIR_TRIPLET_BYTE(write, offset, value)                        \
    do {                                                                 \
        scratch[0] = (value);                                            \
        EEPROM_IO(write, offset, 1);                                     \
        wait_for_eeprom_write();                                         \
    } while (0)

/* If all three values differ, keep the first and leave the EEPROM unchanged. */
#define READ_TRIPLET(read, write, offset)                                \
    do {                                                                 \
        EEPROM_IO(read, offset, TRIPLET_COPY_COUNT);                             \
        stock_r1 = scratch[0];                                           \
        stock_r2 = scratch[1];                                           \
        stock_r3 = scratch[2];                                           \
        if (stock_r1 == stock_r3) {                                      \
            if (stock_r1 != stock_r2) {                                  \
                REPAIR_TRIPLET_BYTE(write, (offset) + 1, stock_r1);      \
            }                                                            \
        } else if (stock_r2 == stock_r3) {                               \
            REPAIR_TRIPLET_BYTE(write, offset, stock_r3);                \
        } else if (stock_r1 == stock_r2) {                               \
            REPAIR_TRIPLET_BYTE(write, (offset) + 2, stock_r1);          \
        }                                                                \
    } while (0)

#endif
