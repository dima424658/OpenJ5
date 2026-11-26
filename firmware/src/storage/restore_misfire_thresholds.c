#include "stock_registers.h"

enum {
    SAVED_STATE_MARKER = 0x55,
    EEPROM_WORD_OFFSET = 0x20,
    EEPROM_SCRATCH_ADDRESS = 0x94,
    COPIES_PER_WORD = 3,
    BYTES_PER_WORD = 2,
    SAVED_WORD_COUNT = 2,
    PRIMARY_LIMIT_LOW = 0,
    PRIMARY_LIMIT_HIGH = 1,
    SECONDARY_SEED_LOW = 2,
    SECONDARY_SEED_HIGH = 3,
};

static __sbit __at (0x12) eeprom_io_failed;
static __sbit __at (0xd7) CY;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __idata __at (EEPROM_SCRATCH_ADDRESS)
    uint8_t saved_word_copies[SAVED_WORD_COUNT][COPIES_PER_WORD][BYTES_PER_WORD];

static volatile __xdata __at (0xfa2b) uint8_t primary_period_dev[2];
static volatile __xdata __at (0xfa2d) uint8_t secondary_period_dev[2];
static volatile __xdata __at (0xfa33) uint8_t peak_period_dev[2];

extern volatile __code const uint8_t period_state_calibration[10];

extern void read_eeprom_page1_bytes(void);
extern void abs_stock_i16(void);
extern void subtract_u16_register_pairs(void);

#define LOAD_PRIMARY_LIMIT()                                              \
    do {                                                                  \
        stock_r2 = period_state_calibration[PRIMARY_LIMIT_LOW];            \
        stock_r3 = period_state_calibration[PRIMARY_LIMIT_HIGH];           \
    } while (0)

#define RESTORE_SAVED_WORD(word, field)                                     \
    do {                                                                   \
        stock_r1 = saved_word_copies[(word)][0][1];                         \
        (field)[1] = stock_r1;                                              \
        stock_r0 = saved_word_copies[(word)][0][0];                         \
        (field)[0] = stock_r0;                                              \
    } while (0)

void restore_misfire_thresholds(void)
{
    if (stock_r5 != SAVED_STATE_MARKER) {
        goto load_calibration;
    }

    stock_r4 = EEPROM_WORD_OFFSET;
    stock_r1 = sizeof(saved_word_copies);
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    read_eeprom_page1_bytes();
    if (CY) {
        eeprom_io_failed = 1;
        goto load_calibration;
    }

    for (uint8_t word = 0; word < SAVED_WORD_COUNT; ++word) {
        for (uint8_t byte = 0; byte < BYTES_PER_WORD; ++byte) {
            for (uint8_t copy = 0; copy < COPIES_PER_WORD - 1; ++copy) {
                if (saved_word_copies[word][copy][byte] !=
                    saved_word_copies[word][copy + 1][byte]) {
                    goto load_calibration;
                }
            }
        }
    }

    RESTORE_SAVED_WORD(0, primary_period_dev);
    abs_stock_i16();
    LOAD_PRIMARY_LIMIT();
    subtract_u16_register_pairs();
    if (!CY) {
        goto load_calibration;
    }

    RESTORE_SAVED_WORD(1, secondary_period_dev);
    abs_stock_i16();
    LOAD_PRIMARY_LIMIT();
    subtract_u16_register_pairs();
    if (!CY) {
        goto load_calibration;
    }

    stock_r0 = primary_period_dev[0];
    stock_r1 = primary_period_dev[1];
    abs_stock_i16();
    stock_r23 = stock_r01;
    stock_r0 = secondary_period_dev[0];
    stock_r1 = secondary_period_dev[1];
    abs_stock_i16();
    peak_period_dev[0] = stock_r0;
    peak_period_dev[1] = stock_r1;

    subtract_u16_register_pairs();
    if (CY) {
        peak_period_dev[0] = stock_r2;
        peak_period_dev[1] = stock_r3;
    }
    return;

load_calibration:
    ACC = period_state_calibration[PRIMARY_LIMIT_LOW];
    peak_period_dev[0] = ACC;
    primary_period_dev[0] = ACC;
    ACC = period_state_calibration[PRIMARY_LIMIT_HIGH];
    peak_period_dev[1] = ACC;
    primary_period_dev[1] = ACC;
    secondary_period_dev[0] = period_state_calibration[SECONDARY_SEED_LOW];
    secondary_period_dev[1] = period_state_calibration[SECONDARY_SEED_HIGH];
}
