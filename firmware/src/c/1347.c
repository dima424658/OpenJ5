#include "stock_registers.h"

static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;

static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xfa4d) uint16_t period_bin_countdown[3];

extern void skip_misfire_period_update(void);

void tick_misfire_period_bin3(void);
void tick_misfire_period_bin4(void);

#define TICK_PERIOD_BIN(index, status_bit)       \
    do {                                         \
        stock_r01 = period_bin_countdown[index]; \
        if (stock_r01 == 0) {                    \
            ACC = misfire_status;                \
            (status_bit) = ACC_BIT0;             \
            misfire_status = ACC;                \
        } else {                                 \
            --stock_r01;                         \
        }                                        \
        period_bin_countdown[index] = stock_r01; \
    } while (0)

void tick_misfire_period_counters(void)
{
    if (ACC_BIT1) {
        TICK_PERIOD_BIN(0, ACC_BIT1);
        skip_misfire_period_update();
        return;
    }
    tick_misfire_period_bin3();
}

void tick_misfire_period_bin3(void)
{
    if (ACC_BIT2) {
        TICK_PERIOD_BIN(1, ACC_BIT2);
        skip_misfire_period_update();
        return;
    }
    tick_misfire_period_bin4();
}

void tick_misfire_period_bin4(void)
{
    if (ACC_BIT3) {
        TICK_PERIOD_BIN(2, ACC_BIT3);
    }
    skip_misfire_period_update();
}
