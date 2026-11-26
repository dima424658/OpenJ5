#include "stock_registers.h"

enum {
    HOLD_BASE_IGNITION = 0x04,
    SIGN_BIAS = 0x80,
    DWELL_ANGLE_SCALE_Q8 = 184,
    DWELL_ANGLE_MIN = 12,
    DWELL_ANGLE_MAX = 174,
    IGNITION_SLOT_DEGREES = 6,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x3e) ignition_coil_order;
static __sbit __at (0x4b) sensor_batch_ready;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0xe0) ACC_BIT0;

static volatile __data __at (0x54) uint8_t rpm_axis_code;
static volatile __data __at (0x62) uint8_t base_ignition;
static volatile __data __at (0x7a) uint8_t ignition_control_7a;
static volatile __xdata __at (0xf800) uint8_t dwell_slots;
static volatile __xdata __at (0xf801) uint8_t dwell_remainder_half_degrees;
static volatile __xdata __at (0xf844) uint8_t previous_ignition;
static volatile __xdata __at (0xf886) uint8_t ignition_advance_ticks;
static volatile __xdata __at (0xf978) uint8_t voltage_quantized;

extern volatile __code const uint8_t ignition_coil_order_raw;
extern volatile __code const uint8_t ignition_min_raw;
extern volatile __code const uint8_t ignition_max_raw;
extern volatile __code const uint8_t ignition_advance_period;
extern volatile __code const uint8_t ignition_max_advance_step;
extern volatile __code const uint8_t ignition_delay_compensation;
extern __code const uint8_t ignition_dwell_raw[16];

extern void limit_stock_i8_by_sign(void);
extern void clamp_stock_u8(void);
extern void interp_descending_curve_q4(void);
extern void write_ignition_timing_slots(void);

void calculate_ignition_dwell_slots(void);

void continue_ignition_output(void)
{
    ACC = ignition_coil_order_raw;
    ignition_coil_order = ACC_BIT0;

    stock_r0 = ignition_min_raw;
    stock_r1 = ignition_max_raw;
    ACC = base_ignition;
    limit_stock_i8_by_sign();
    base_ignition = ACC;

    if (sensor_batch_ready && !idle_regulating && !(ignition_control_7a & HOLD_BASE_IGNITION)) {
        B = ignition_advance_period;
        ACC = ignition_advance_ticks + 1;
        if (ACC != 0) {
            ignition_advance_ticks = ACC;
        }
        if (ACC < B) {
            B = 0;
        } else {
            ignition_advance_ticks = 0;
            B = ignition_max_advance_step;
        }

        // Bias signed angles into unsigned order before comparing them.
        stock_r0 = base_ignition + SIGN_BIAS;
        ACC = previous_ignition + SIGN_BIAS;
        if (ACC < stock_r0) {
            ACC = previous_ignition + SIGN_BIAS + B;
            if (ACC < stock_r0) {
                base_ignition += ACC - stock_r0;
            }
        }
    }
    calculate_ignition_dwell_slots();
}

void calculate_ignition_dwell_slots(void)
{
    previous_ignition = base_ignition;
    B = rpm_axis_code;
    ACC = ignition_delay_compensation;
    B = (uint16_t)ACC * B >> 8;
    stock_r2 = B + base_ignition;

    ACC = voltage_quantized;
    STOCK_DPTR = (uint16_t)ignition_dwell_raw;
    interp_descending_curve_q4();
    B = ACC;
    const uint16_t dwell_angle = (uint16_t)rpm_axis_code * B >> 6;
    ACC = dwell_angle > UINT8_MAX ? UINT8_MAX : (uint8_t)dwell_angle;
    B = DWELL_ANGLE_SCALE_Q8;
    ACC = (uint16_t)ACC * B >> 8;
    stock_r0 = DWELL_ANGLE_MIN;
    stock_r1 = DWELL_ANGLE_MAX;
    clamp_stock_u8();

    B = ACC % IGNITION_SLOT_DEGREES;
    ACC /= IGNITION_SLOT_DEGREES;
    dwell_slots = ACC;
    dwell_remainder_half_degrees = 2 * B;
    write_ignition_timing_slots();
}
