#include "stock_registers.h"

enum {
    PERIOD_HISTORY_LOW = 0x35,
    PERIOD_HISTORY_HIGH = 0x41,
};

#define HISTORY_BYTE(offset) \
    (*(volatile __xdata uint8_t *)(UINT16_C(0xfa00) | (uint8_t)(offset)))

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sbit __at (0xf7) B_CARRY;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (6) uint8_t stock_r6;
static volatile __data __at (7) uint8_t stock_r7;
static volatile __data __at (4) uint16_t stock_r45;
static volatile __data __at (6) uint16_t stock_r67;
static volatile __data __at (4) uint32_t stock_r4567;

static volatile __xdata __at (0xf940) uint8_t misfire_rpm_bin_offset;
static volatile __xdata __at (0xf941) uint8_t misfire_rpm_bin_mask;
static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xfa0f) uint16_t selected_period_ratio;

extern volatile __code const uint8_t misfire_period_weight[2];
extern volatile __code const uint8_t misfire_period_alt_weight[2];

extern void negate_u16_register_pair(void);
extern void update_misfire_period_bin1(void);
extern void tick_misfire_period_counters(void);

void mix_misfire_period_ratio(void);

#define LOAD_PERIOD_WEIGHT(low, high)              \
    do {                                           \
        if (misfire_status & 1) {                  \
            (low) = misfire_period_alt_weight[0];  \
            (high) = misfire_period_alt_weight[1]; \
        } else {                                   \
            (low) = misfire_period_weight[0];      \
            (high) = misfire_period_weight[1];     \
        }                                          \
    } while (0)

#define READ_HISTORY(base)                                      \
    do {                                                        \
        const uint8_t offset = (base) + misfire_rpm_bin_offset; \
        stock_r2 = HISTORY_BYTE(offset);                        \
        stock_r3 = HISTORY_BYTE(offset + 1);                    \
    } while (0)

#define WRITE_HISTORY(base, low, high)                          \
    do {                                                        \
        const uint8_t offset = (base) + misfire_rpm_bin_offset; \
        HISTORY_BYTE(offset) = (low);                           \
        HISTORY_BYTE(offset + 1) = (high);                      \
    } while (0)

#define MDU_MULTIPLY_REGISTER_PAIRS() \
    do {                              \
        mdu_0 = stock_r2;             \
        mdu_4 = stock_r0;             \
        mdu_1 = stock_r3;             \
        mdu_5 = stock_r1;             \
        __asm nop __endasm;           \
        __asm nop __endasm;           \
        __asm nop __endasm;           \
        __asm nop __endasm;           \
    } while (0)

#define READ_MDU_PRODUCT(low, middle_low, middle_high, high) \
    do {                                                     \
        (low) = mdu_0;                                       \
        (middle_low) = mdu_1;                                \
        (middle_high) = mdu_2;                               \
        (high) = mdu_3;                                      \
    } while (0)

void continue_misfire_period_update(void)
{
    LOAD_PERIOD_WEIGHT(stock_r0, stock_r1);
    negate_u16_register_pair();

    // The old 32-bit per-bin value is stored in two parallel word tables.
    READ_HISTORY(PERIOD_HISTORY_HIGH);
    MDU_MULTIPLY_REGISTER_PAIRS();
    READ_MDU_PRODUCT(stock_r4, stock_r5, stock_r6, stock_r7);
    READ_HISTORY(PERIOD_HISTORY_LOW);
    MDU_MULTIPLY_REGISTER_PAIRS();
    READ_MDU_PRODUCT(stock_r0, stock_r1, stock_r2, stock_r3);
    // Keep only the high word of the low-half product after the Q16 shift.
    stock_r4567 += stock_r23;
    mix_misfire_period_ratio();
}

void mix_misfire_period_ratio(void)
{
    stock_r01 = selected_period_ratio;
    LOAD_PERIOD_WEIGHT(stock_r2, stock_r3);
    MDU_MULTIPLY_REGISTER_PAIRS();
    READ_MDU_PRODUCT(stock_r0, stock_r1, stock_r2, stock_r3);

    const uint32_t low_sum = (uint32_t)stock_r01 + stock_r45;
    stock_r01 = (uint16_t)low_sum;
    B_CARRY = (low_sum >> 16) != 0;
    WRITE_HISTORY(PERIOD_HISTORY_LOW, stock_r0, stock_r1);
    stock_r23 += stock_r67 + B_CARRY;
    WRITE_HISTORY(PERIOD_HISTORY_HIGH, stock_r2, stock_r3);

    ACC = misfire_rpm_bin_mask;
    if (ACC & 1) {
        update_misfire_period_bin1();
        return;
    }
    tick_misfire_period_counters();
}
