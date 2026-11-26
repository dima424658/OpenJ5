#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xf0) B;
static __sbit __at (0x46) full_barometric_correction;
static __sbit __at (0x4b) sensor_batch_ready;
static __sbit __at (0x59) idle_mode;

static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __xdata __at (0xf82c) uint16_t measured_air_charge;
static volatile __xdata __at (0xf830) uint16_t air_charge_limit;
static volatile __xdata __at (0xf864) uint16_t airflow_period_charge;
static volatile __xdata __at (0xf866) uint16_t turbo_charge_history;
static volatile __xdata __at (0xf8bc) uint16_t bounded_air_charge;
static volatile __xdata __at (0xf949) uint8_t charge_filter_coefficient;

extern volatile __code const uint8_t turbo_charge_filter_gain;
extern volatile __code const uint8_t idle_charge_filter_gain;
extern volatile __code const uint8_t maximum_cylinder_charge[2];
extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t cylinder_charge_correction[256];

extern void multiply_stock_u16_u8(void);
extern void multiply_stock_i16_u8(void);
extern void shift_left_saturating_1(void);
extern void shift_left_saturating_4(void);
extern void subtract_u16_register_pairs(void);
extern void add_u16_register_pairs(void);
extern void minimum_u16_register_pair(void);
extern void interp_bilinear_u8(void);
extern void process_cylinder_charge(void);

void filter_measured_air_charge(void);
void apply_cylinder_charge_limit(void);

void continue_charge_after_base(void)
{
    if (!sensor_batch_ready) {
        apply_cylinder_charge_limit();
        return;
    }

    if (turbo_charge_filter_gain != 0) {
        B = rpm_eighth;
        multiply_stock_u16_u8();
        shift_left_saturating_4();
        stock_r23 = turbo_charge_history;
        turbo_charge_history = stock_r01;
        subtract_u16_register_pairs();
        B = turbo_charge_filter_gain;
        multiply_stock_i16_u8();
        stock_r23 = airflow_period_charge;
        add_u16_register_pairs();
        airflow_period_charge = stock_r01;
    }

    filter_measured_air_charge();
}

void filter_measured_air_charge(void)
{
    if (full_barometric_correction) {
        stock_r01 = air_charge_limit;
    } else {
        stock_r23 = measured_air_charge;
        subtract_u16_register_pairs();
        B = idle_mode ? idle_charge_filter_gain : charge_filter_coefficient;
        multiply_stock_i16_u8();
        stock_r23 = measured_air_charge;
        add_u16_register_pairs();
    }

    apply_cylinder_charge_limit();
}

void apply_cylinder_charge_limit(void)
{
    measured_air_charge = stock_r01;
    stock_r23 = air_charge_limit;
    if (stock_r23 != UINT16_MAX) {
        // In the crank interrupt's bank 1, this physical pair aliases R3:R2.
        stock_r01 = bank_one_r23;
        process_cylinder_charge();
        return;
    }

    stock_r01 = measured_air_charge;
    stock_r3 = maximum_cylinder_charge[1];
    stock_r2 = maximum_cylinder_charge[0];
    minimum_u16_register_pair();
    bounded_air_charge = stock_r01;

    stock_r3 = throttle_quantization_q4[throttle_position];
    STOCK_DPTR = (uint16_t)cylinder_charge_correction;
    stock_r2 = rpm_q4;
    interp_bilinear_u8();
    B = ACC;
    stock_r01 = bounded_air_charge;
    multiply_stock_u16_u8();
    shift_left_saturating_1();
    process_cylinder_charge();
}
