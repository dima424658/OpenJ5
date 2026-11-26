#include "stock_registers.h"

enum {
    RAW_PERIOD_BASE = 0xb0,
    PERIOD_HISTORY_LENGTH = 7,
};

#define RAW_PERIOD_ADDRESS(lag)                          \
    ((uint8_t)(RAW_PERIOD_BASE + 2 * (stock_r5 - (lag) + \
        (stock_r5 < (lag) ? PERIOD_HISTORY_LENGTH : 0))))
#define RAW_PERIOD_BYTE(address) (*(volatile __idata uint8_t *)(address))

static __sbit __at (0x5b) deceleration_fuel_cut;
static volatile __data __at (0x43) uint8_t knock_mask_index;
static volatile __data __at (0x05) uint8_t stock_r5;
static volatile __data __at (0x06) uint16_t stock_r67;
static volatile __data __at (0x08) uint16_t bank_one_r01;
static volatile __data __at (0x0e) uint16_t bank_one_r67;

static volatile __xdata __at (0xf93e) uint8_t crank_diagnostic_flags;
static volatile __xdata __at (0xfa5d) uint8_t misfire_fuel_cut_events;
static volatile __xdata __at (0xf99c) uint8_t raw_period_delta;
static volatile __xdata __at (0xf99d) uint8_t early_period_offset;
static volatile __xdata __at (0xf99e) uint8_t late_period_offset;
static volatile __xdata __at (0xf99f) uint16_t early_period_residual;
static volatile __xdata __at (0xf9a1) uint16_t late_period_residual;
static volatile __xdata __at (0xf9cf) uint16_t early_raw_period;
static volatile __xdata __at (0xf9cb) uint16_t late_raw_period;

extern volatile __code const uint8_t misfire_fuel_cut_event_limit;

extern void skip_misfire_period_scaling(void);
extern void skip_misfire_period_update(void);
extern void combine_misfire_event_flags(void);
extern void add_u16_register_pairs(void);
extern void subtract_u16_register_pairs(void);
extern void select_misfire_period_residual(void);

void build_misfire_period_residuals(void);

void check_misfire_fuel_cut(void)
{
    if (!deceleration_fuel_cut || !(crank_diagnostic_flags & 0x02)) {
        skip_misfire_period_scaling();
        return;
    }
    uint8_t fuel_cut_events = misfire_fuel_cut_events;
    if (fuel_cut_events < UINT8_MAX) {
        ++fuel_cut_events;
    }
    misfire_fuel_cut_events = fuel_cut_events;
    if (fuel_cut_events <= misfire_fuel_cut_event_limit ||
        (knock_mask_index & 1)) {
        combine_misfire_event_flags();
        return;
    }

    stock_r0 = RAW_PERIOD_ADDRESS(1);
    stock_r1 = RAW_PERIOD_ADDRESS(5);
    const uint8_t newest_low = RAW_PERIOD_BYTE(stock_r0);
    const uint8_t oldest_low = RAW_PERIOD_BYTE(stock_r1);
    const uint8_t newest_high = RAW_PERIOD_BYTE(++stock_r0);
    const uint8_t oldest_high = RAW_PERIOD_BYTE(++stock_r1);
    bank_one_r67 = ((uint16_t)oldest_high << 8) | oldest_low;
    const uint16_t newest = ((uint16_t)newest_high << 8) | newest_low;
    const uint16_t oldest = ((uint16_t)oldest_high << 8) | oldest_low;
    const uint16_t delta = newest - oldest;
    stock_r2 = delta;
    if (newest < oldest) {
        raw_period_delta = 0;
        skip_misfire_period_update();
        return;
    }
    if (delta > UINT8_MAX) {
        skip_misfire_period_update();
        return;
    }

    raw_period_delta = stock_r2;
    const uint16_t scaled_delta = 3 * (uint16_t)stock_r2;
    const uint8_t bounded_delta = scaled_delta > UINT8_MAX ? UINT8_MAX : scaled_delta;
    // The rounding addition wraps in the accumulator before division.
    late_period_offset = (uint8_t)(bounded_delta + 2) / 4;
    early_period_offset = raw_period_delta - late_period_offset;
    build_misfire_period_residuals();
}

#define READ_RAW_PERIOD(lag)                    \
    do {                                        \
        stock_r0 = RAW_PERIOD_ADDRESS(lag);     \
        stock_r2 = RAW_PERIOD_BYTE(stock_r0);   \
        stock_r3 = RAW_PERIOD_BYTE(++stock_r0); \
    } while (0)

void build_misfire_period_residuals(void)
{
    // In bank 1, R7:R6 holds p[n-5] and the physical scratch pair is R1:R0.
    READ_RAW_PERIOD(4);
    early_raw_period = stock_r23;
    bank_one_r01 = stock_r67;
    subtract_u16_register_pairs();
    stock_r23 = early_period_offset;
    add_u16_register_pairs();
    early_period_residual = stock_r01;

    READ_RAW_PERIOD(2);
    late_raw_period = stock_r23;
    bank_one_r01 = stock_r67;
    subtract_u16_register_pairs();
    stock_r23 = late_period_offset;
    add_u16_register_pairs();
    late_period_residual = stock_r01;
    select_misfire_period_residual();
}
