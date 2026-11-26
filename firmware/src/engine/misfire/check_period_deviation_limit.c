#include "stock_registers.h"

enum {
    PEAK_SEED,
    PRIMARY_SEED,
    SECONDARY_SEED_LOW,
    SECONDARY_SEED_HIGH,
    EXIT_LIMIT_LOW,
    EXIT_LIMIT_HIGH,
    ENTRY_LIMIT_LOW,
    ENTRY_LIMIT_HIGH,
    COUNT_SEED_LOW,
    COUNT_SEED_HIGH,
    PERIOD_BIN_COUNT = 6,
};

static __sfr __at (0xf0) B;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xd7) CY;

static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xfa0f) uint8_t selected_period_ratio[2];
static volatile __xdata __at (0xfa15) uint8_t period_offset[2];
static volatile __xdata __at (0xfa17) uint8_t period_scale[2];
static volatile __xdata __at (0xfa2b) uint8_t primary_period_dev[2];
static volatile __xdata __at (0xfa2d) uint8_t secondary_period_dev[2];
static volatile __xdata __at (0xfa33) uint8_t peak_period_low;
static volatile __xdata __at (0xfa35) uint8_t period_history_low[2 * PERIOD_BIN_COUNT];
static volatile __xdata __at (0xfa41) uint8_t period_history_high[2 * PERIOD_BIN_COUNT];
static volatile __xdata __at (0xfa4d) uint8_t period_bin_countdown[3][2];

extern volatile __code const uint8_t period_state_calibration[10];

extern void subtract_u16_register_pairs(void);
extern void skip_misfire_period_update(void);

void reset_misfire_period_history(void);

void check_period_deviation_limit(void)
{
    ACC = misfire_status;
    B = ACC;
    if (!ACC_BIT0) {
        stock_r2 = period_state_calibration[ENTRY_LIMIT_LOW];
        stock_r3 = period_state_calibration[ENTRY_LIMIT_HIGH];
        subtract_u16_register_pairs();
        ACC = B;
        ACC_BIT0 = CY;
        misfire_status = ACC;
        skip_misfire_period_update();
        return;
    }

    stock_r2 = period_state_calibration[EXIT_LIMIT_LOW];
    stock_r3 = period_state_calibration[EXIT_LIMIT_HIGH];
    subtract_u16_register_pairs();
    if (CY) {
        skip_misfire_period_update();
        return;
    }

    reset_misfire_period_history();
}

void reset_misfire_period_history(void)
{
    misfire_status = B & 0xf0;

    ACC = period_state_calibration[COUNT_SEED_LOW];
    period_bin_countdown[0][0] = ACC;
    period_bin_countdown[1][0] = ACC;
    period_bin_countdown[2][0] = ACC;
    ACC = period_state_calibration[COUNT_SEED_HIGH];
    period_bin_countdown[0][1] = ACC;
    period_bin_countdown[1][1] = ACC;
    period_bin_countdown[2][1] = ACC;

    ACC = period_state_calibration[PEAK_SEED];
    peak_period_low = ACC;
    primary_period_dev[0] = ACC;
    ACC = period_state_calibration[PRIMARY_SEED];
    primary_period_dev[0] = ACC;
    primary_period_dev[1] = ACC;
    secondary_period_dev[0] = period_state_calibration[SECONDARY_SEED_LOW];
    secondary_period_dev[1] = period_state_calibration[SECONDARY_SEED_HIGH];

    period_offset[0] = 0;
    period_offset[1] = 0;
    period_scale[0] = 0;
    period_scale[1] = 0x80;
    for (uint8_t i = 0; i < 2 * PERIOD_BIN_COUNT; ++i) {
        period_history_low[i] = 0;
    }
    for (uint8_t i = 0; i < 2 * PERIOD_BIN_COUNT; i += 2) {
        period_history_high[i] = 0;
    }
    for (uint8_t i = 1; i < 2 * PERIOD_BIN_COUNT; i += 2) {
        period_history_high[i] = 0x80;
    }
    selected_period_ratio[0] = 0;
    selected_period_ratio[1] = 0x80;
    skip_misfire_period_update();
}
