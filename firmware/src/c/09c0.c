#include "stock_registers.h"

enum {
    PERIOD_BASE_INDEX = 0xb0,
    PERIOD_HISTORY_LENGTH = 7,
    PERIOD_BASE_LOW = 0x19,
};

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sbit __at (0x5b) deceleration_fuel_cut;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x05) uint8_t stock_r5;
static volatile __data __at (0x06) uint16_t stock_r67;
static volatile __idata __at (0xbe) uint8_t previous_crank_period_index;
static volatile __xdata __at (0xf9d7) uint16_t period_positive_delta;
static volatile __xdata __at (0xf9dd) uint16_t period_delta_quarter;
static volatile __xdata __at (0xf9ed) uint16_t corrected_period_delta;
static volatile __xdata __at (0xf9f3) uint16_t period_absolute_delta;

extern void subtract_u16_register_pairs(void);
extern void calculate_misfire_period_score(void);
extern void continue_misfire_period_checks(void);

void correct_misfire_period_delta(void);

#define PERIOD_PAGE UINT16_C(0xfa00)

#define READ_PERIOD_AT_INDEX(index)                                      \
    (*(volatile __xdata uint16_t *)(PERIOD_PAGE |                          \
        (uint8_t)(PERIOD_BASE_LOW + 2 * (index))))

#define READ_PERIOD_LAG(lag)                                             \
    READ_PERIOD_AT_INDEX(stock_r5 - (lag) +                               \
        (stock_r5 < (lag) ? PERIOD_HISTORY_LENGTH : 0))

void continue_misfire_period_history(void)
{
    stock_r5 = (uint8_t)(previous_crank_period_index - PERIOD_BASE_INDEX) >> 1;

    // The stock branch on fuel cut joins at the next instruction.
    (void)deceleration_fuel_cut;
    stock_r23 = READ_PERIOD_LAG(3);
    stock_r67 = stock_r23;
    stock_r01 = READ_PERIOD_LAG(2);
    subtract_u16_register_pairs();
    if (CY) {
        stock_r01 = 0;
        period_positive_delta = 0;
        continue_misfire_period_checks();
        return;
    }
    correct_misfire_period_delta();
}

void correct_misfire_period_delta(void)
{
    period_positive_delta = stock_r01;
    stock_r01 = READ_PERIOD_LAG(4);
    stock_r23 = READ_PERIOD_AT_INDEX(stock_r5);
    subtract_u16_register_pairs();
    if (CY) {
        stock_r01 = -stock_r01;
    }
    period_absolute_delta = stock_r01;

    mdu_0 = stock_r0;
    mdu_1 = stock_r1;
    mdu_4 = 4;
    mdu_5 = 0;
    __asm
        nop
        nop
        nop
        nop
    __endasm;
    stock_r23 = ((uint16_t)mdu_1 << 8) | mdu_0;
    (void)mdu_4;
    (void)mdu_5;
    period_delta_quarter = stock_r23;

    stock_r01 = period_positive_delta;
    subtract_u16_register_pairs();
    if (CY) {
        stock_r01 = 0;
        continue_misfire_period_checks();
        return;
    }
    corrected_period_delta = stock_r01;
    calculate_misfire_period_score();
}
