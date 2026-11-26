#include "stock_registers.h"

enum { AIRFLOW_LIMIT_Q3 = 240, EXHAUST_CURVE_POINTS = 32 };

static __sfr __at (0xf0) B;
static __sbit __at (0x5b) deceleration_fuel_cut;

static volatile __xdata __at (0xf808) uint16_t mass_airflow;
static volatile __xdata __at (0xf879) uint8_t exhaust_airflow_q3;
static volatile __xdata __at (0xf87a) uint8_t exhaust_airflow_index;
static volatile __xdata __at (0xf892) uint16_t exhaust_temperature_target;
static volatile __xdata __at (0xf894) uint16_t exhaust_temperature_filtered;

extern volatile __code const uint8_t exhaust_airflow_scale_q8;
extern volatile __code const uint8_t exhaust_temperature_fuel_cut;
extern __code const uint8_t exhaust_temperature_running[EXHAUST_CURVE_POINTS];
extern __code const uint8_t exhaust_pipe_filter_q8[EXHAUST_CURVE_POINTS];

extern void multiply_stock_u16_u8(void);
extern void multiply_stock_i16_u8(void);
extern void subtract_u16_register_pairs(void);
extern void add_u16_register_pairs(void);

void update_exhaust_temperature(void)
{
    stock_r01 = mass_airflow;
    B = exhaust_airflow_scale_q8;
    multiply_stock_u16_u8();

    if (stock_r1 != 0 || stock_r0 >= AIRFLOW_LIMIT_Q3) {
        ACC = AIRFLOW_LIMIT_Q3;
    } else {
        ACC = stock_r0;
    }
    exhaust_airflow_q3 = ACC;
    ACC = (uint8_t)(ACC + 4) >> 3;
    exhaust_airflow_index = ACC;
    B = ACC;

    if (deceleration_fuel_cut) {
        ACC = exhaust_temperature_fuel_cut;
    } else {
        ACC = exhaust_temperature_running[ACC];
    }
    stock_r1 = ACC;
    ACC = B;
    B = exhaust_pipe_filter_q8[ACC];

    // R1 is the target's integer part and R0 is its fractional part.
    stock_r0 = 0;
    exhaust_temperature_target = stock_r01;

    // Scale the signed Q8 error, then add the correction to the previous estimate.
    stock_r23 = exhaust_temperature_filtered;
    subtract_u16_register_pairs();
    multiply_stock_i16_u8();

    stock_r23 = exhaust_temperature_filtered;
    add_u16_register_pairs();
    exhaust_temperature_filtered = stock_r01;
}
