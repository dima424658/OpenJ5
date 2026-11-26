#include "stock_registers.h"

enum {
    PRIMARY_DEV_LOW = 0,
    PRIMARY_DEV_HIGH = 1,
    SECONDARY_DEV_LOW = 2,
    SECONDARY_DEV_HIGH = 3,
    COUNT_SEED_LOW = 8,
    COUNT_SEED_HIGH = 9,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xfa0f) uint8_t selected_period_ratio[2];
static volatile __xdata __at (0xfa15) uint8_t period_offset[2];
static volatile __xdata __at (0xfa17) uint8_t period_scale[2];
static volatile __xdata __at (0xfa2b) uint8_t primary_period_dev[2];
static volatile __xdata __at (0xfa2d) uint8_t secondary_period_dev[2];
static volatile __xdata __at (0xfa33) uint8_t peak_period_dev[2];
static volatile __xdata __at (0xfa35) uint8_t period_history_low[12];
static volatile __xdata __at (0xfa41) uint8_t period_history_high[12];
static volatile __xdata __at (0xfa4d) uint8_t period_bin_countdown[3][2];

extern volatile __code const uint8_t period_state_calibration[10];

void reset_saved_state_ac3c(void)
{
    ACC = 0;
    period_offset[0] = ACC;
    period_offset[1] = ACC;
    period_scale[0] = ACC;
    selected_period_ratio[0] = ACC;

    B = sizeof(period_history_low);
    STOCK_DPTR = (uint16_t)period_history_low;
    do {
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;
    } while (--B);

    /* Only the first and last high-history words receive a seed here. */
    period_history_high[0] = ACC;
    period_history_high[10] = ACC;
    ACC = 0x80;
    period_scale[1] = ACC;
    selected_period_ratio[1] = ACC;
    period_history_high[1] = ACC;
    period_history_high[11] = ACC;

    ACC = period_state_calibration[COUNT_SEED_LOW];
    period_bin_countdown[0][0] = ACC;
    period_bin_countdown[1][0] = ACC;
    period_bin_countdown[2][0] = ACC;
    ACC = period_state_calibration[COUNT_SEED_HIGH];
    period_bin_countdown[0][1] = ACC;
    period_bin_countdown[1][1] = ACC;
    period_bin_countdown[2][1] = ACC;
}

void reset_saved_state_ac95(void)
{
    ACC = 0;
    period_history_high[2] = ACC;
    period_history_high[4] = ACC;
    period_history_high[6] = ACC;
    period_history_high[8] = ACC;
    ACC = 0x80;
    period_history_high[3] = ACC;
    period_history_high[5] = ACC;
    period_history_high[7] = ACC;
    period_history_high[9] = ACC;

    ACC = period_state_calibration[PRIMARY_DEV_LOW];
    peak_period_dev[0] = ACC;
    primary_period_dev[0] = ACC;
    ACC = period_state_calibration[PRIMARY_DEV_HIGH];
    peak_period_dev[1] = ACC;
    primary_period_dev[1] = ACC;
    secondary_period_dev[0] = period_state_calibration[SECONDARY_DEV_LOW];
    secondary_period_dev[1] = period_state_calibration[SECONDARY_DEV_HIGH];
    misfire_status = 0;
}
