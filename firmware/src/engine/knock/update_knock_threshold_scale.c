#include "stock_registers.h"

enum {
    KNOCK_CORRECTION_BIAS = 128,
    KNOCK_LIMIT_BASE = 254,
};

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xf0) B;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x4f) uint8_t knock_scale_4f;
static volatile __data __at (0x42) uint8_t knock_limit_offset_42;
static volatile __xdata __at (0xf937) uint8_t knock_threshold_correction;

extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t knock_threshold_correction_raw[256];
extern void interp_bilinear_u8(void);
extern void multiply_stock_u16_u8(void);
extern void update_knock_rpm_parameters(void);

void update_knock_threshold_scale(void)
{
    stock_r2 = rpm_q4;
    stock_r3 = throttle_quantization_q4[throttle_position];
    STOCK_DPTR = (uint16_t)knock_threshold_correction_raw;
    interp_bilinear_u8();

    /* The multiplier takes R1:R0 and B, then leaves the 24-bit product
     * in R1:R0:B. Saturate the upper word to the range of one byte.
     */
    stock_r01 = KNOCK_CORRECTION_BIAS + (uint16_t)ACC;
    B = knock_scale_4f;
    multiply_stock_u16_u8();
    if (stock_r1 != 0) {
        stock_r0 = UINT8_MAX;
    }

    const uint8_t limit = (uint8_t)(KNOCK_LIMIT_BASE - knock_limit_offset_42);
    const uint8_t scaled = stock_r0;
    knock_threshold_correction = scaled < limit ? scaled : limit;

    update_knock_rpm_parameters();
}
