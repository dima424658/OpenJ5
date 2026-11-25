#include "stock_registers.h"

enum {
    FUEL_WEIGHT_BASELINE = 128,
    Q4_HALF_STEP = 8,
    Q4_INTEGER_MASK = 0xf0,
};

static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __xdata __at (0xf887) uint8_t coolant_axis_index;
static volatile __xdata __at (0xf888) uint8_t coolant_axis_fraction;
static volatile __xdata __at (0xf97f) uint8_t fuel_weighted_correction;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x59) uint8_t rpm_nearest_bin;

extern __code const uint8_t fuel_weight_by_temp_raw[39];
extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t fuel_weight_by_throttle_raw[256];
extern void interp_curve_u8(void);
extern void update_load_warmup_ignition(void);

void update_fuel_weighting(void)
{
    /* The stock curve routine takes DPTR and R2:R3 and returns its byte in A. */
    stock_r2 = coolant_axis_index;
    stock_r3 = coolant_axis_fraction;
    STOCK_DPTR = (uint16_t)fuel_weight_by_temp_raw;
    interp_curve_u8();

    const uint8_t interpolated = ACC;
    const uint8_t increment = interpolated >= FUEL_WEIGHT_BASELINE
        ? interpolated - FUEL_WEIGHT_BASELINE : 0;
    const uint8_t throttle = throttle_quantization_q4[throttle_position];
    const uint8_t row_offset = (uint8_t)(throttle + Q4_HALF_STEP) & Q4_INTEGER_MASK;
    const uint8_t weight = fuel_weight_by_throttle_raw[(uint8_t)(row_offset + rpm_nearest_bin)];
    fuel_weighted_correction = FUEL_WEIGHT_BASELINE
        + (((uint16_t)increment * weight + 128u) >> 8);

    update_load_warmup_ignition();
}
