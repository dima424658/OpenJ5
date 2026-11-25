#include "stock_registers.h"

enum { CHARGE_Q4_MAX = 0xf0 };

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x50) cranking_fuel_selected;
static __sbit __at (0x51) startup_flag_51;
static __sbit __at (0x55) fuel_mode_flag_55;
static __sbit __at (0x56) fuel_mode_flag_56;

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x5d) uint8_t charge_nearest_half_bin;
static volatile __data __at (0x5e) uint8_t charge_nearest_bin;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __xdata __at (0xf829) uint8_t cranking_cycle_index;
static volatile __xdata __at (0xf82e) uint16_t cylinder_charge;
static volatile __xdata __at (0xf837) uint16_t cranking_fuel;
static volatile __xdata __at (0xf89b) uint16_t poststart_fuel_floor;
static volatile __xdata __at (0xf89e) uint8_t cranking_events;
static volatile __xdata __at (0xf8af) uint8_t rpm_eighth_bin;
static volatile __xdata __at (0xf8b8) uint8_t previous_throttle_position;
static volatile __xdata __at (0xf94a) uint8_t charge_delta_magnitude;
static volatile __xdata __at (0xf98c) uint8_t crank_throttle_position;
static volatile __xdata __at (0xffc0) uint8_t retained_barometric_correction;

extern volatile __code const uint8_t charge_axis_minimum[2];
extern volatile __code const uint8_t charge_axis_gain;
extern volatile __code const uint8_t cranking_large_cycle_count;
extern volatile __code const uint8_t cranking_cycle_period;
extern __code const uint8_t startup_exit_rpm_by_coolant[39];
extern __code const uint8_t cranking_small_fuel_raw[39];
extern __code const uint8_t cranking_large_fuel_raw[39];
extern __code const uint8_t cranking_rpm_correction_raw[32];
extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t cranking_throttle_correction[16];
extern __code const uint8_t cranking_speed_correction[32];

extern void abs_accumulator_i8(void);
extern void subtract_u16_register_pairs(void);
extern void multiply_stock_u16_u8(void);
extern void shift_right_u16_3(void);
extern void shift_left_saturating_1(void);
extern void finish_crank_without_fuel(void);
extern void calculate_running_fuel(void);
extern void apply_fuel_corrections(void);

void select_crank_fuel_path(void);
void calculate_current_cranking(void);
void apply_large_cranking_fuel(void);
void publish_cranking_fuel(void);

void finish_throttle_charge_update(void)
{
    previous_throttle_position = crank_throttle_position;
    stock_r01 = cylinder_charge;
    stock_r2 = charge_axis_minimum[0];
    stock_r3 = charge_axis_minimum[1];
    subtract_u16_register_pairs();
    ACC = 0;
    if (!CY) {
        B = charge_axis_gain;
        shift_right_u16_3();
        multiply_stock_u16_u8();
        ACC = stock_r01 < CHARGE_Q4_MAX ? stock_r0 : CHARGE_Q4_MAX;
    }
    B = ACC;
    ACC -= charge_q4;
    abs_accumulator_i8();
    charge_delta_magnitude = ACC;
    charge_q4 = B;
    charge_nearest_half_bin = (uint8_t)(B + 4) >> 3;
    B = (charge_q4 + 8) & 0xf0;
    charge_nearest_bin = B >> 4;
    operating_point_index = B + ((uint8_t)(rpm_q4 + 8) >> 4);
    select_crank_fuel_path();
}

#define APPLY_CRANKING_GAINS()                                     \
    do {                                                           \
        ACC = throttle_quantization_q4[throttle_position];         \
        B = cranking_throttle_correction[(uint8_t)(ACC + 8) >> 4]; \
        multiply_stock_u16_u8();                                   \
        B = cranking_speed_correction[cranking_events >> 3];       \
        multiply_stock_u16_u8();                                   \
        B = retained_barometric_correction;                        \
        multiply_stock_u16_u8();                                   \
        shift_left_saturating_1();                                 \
    } while (0)

void select_crank_fuel_path(void)
{
    stock_r01 = cylinder_charge;
    if (!fuel_mode_flag_56) {
        finish_crank_without_fuel();
        return;
    }
    if (!fuel_mode_flag_55) {
        calculate_running_fuel();
        return;
    }

    ACC = cranking_events + 1;
    if (ACC != 0) {
        cranking_events = ACC;
    }
    stock_r0 = 0;
    stock_r1 = cranking_small_fuel_raw[coolant_nearest_index];
    B = cranking_rpm_correction_raw[startup_exit_rpm_by_coolant[coolant_nearest_index] / 4];
    multiply_stock_u16_u8();
    APPLY_CRANKING_GAINS();
    poststart_fuel_floor = stock_r01;
    calculate_current_cranking();
}

void calculate_current_cranking(void)
{
    stock_r0 = 0;
    stock_r1 = cranking_small_fuel_raw[coolant_nearest_index];
    B = cranking_rpm_correction_raw[rpm_eighth_bin];
    multiply_stock_u16_u8();
    APPLY_CRANKING_GAINS();
    apply_large_cranking_fuel();
}

void apply_large_cranking_fuel(void)
{
    if (startup_flag_51) {
        if (cranking_cycle_index >= cranking_cycle_period) {
            cranking_cycle_index = 0;
        }
        ACC = cranking_cycle_index;
        cranking_cycle_index = ACC + 1;
        if (ACC < cranking_large_cycle_count) {
            stock_r0 = 0;
            stock_r1 = cranking_large_fuel_raw[coolant_nearest_index];
            APPLY_CRANKING_GAINS();
        }
    }
    publish_cranking_fuel();
}

void publish_cranking_fuel(void)
{
    cranking_fuel = stock_r01;
    cranking_fuel_selected = 1;
    apply_fuel_corrections();
}
