#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x2f) idle_adaptation_ready_2f;
static __sbit __at (0x59) idle_regulating;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x6b) uint8_t fuel_cut_phase;
static volatile __xdata __at (0xf97b) uint16_t poststart_fuel_decay;
static volatile __xdata __at (0xf986) uint8_t idle_adaptation_count_f986;

extern volatile __code const uint8_t poststart_fuel_decay_gain;
extern __code const uint8_t poststart_fuel_decay_by_temp[39];
extern void shift_right_u16_3(void);
extern void apply_idle_airflow(void);

void run_airflow_control(void);

void leave_idle_for_load(void)
{
    idle_regulating = 0;
    fuel_cut_phase = 0;
    idle_adaptation_ready_2f = 0;
    idle_adaptation_count_f986 = 0;

    B = poststart_fuel_decay_gain;
    stock_r01 = (uint16_t)poststart_fuel_decay_by_temp[coolant_nearest_index] * B;
    shift_right_u16_3();
    poststart_fuel_decay = stock_r01;

    run_airflow_control();
}

void run_airflow_control(void)
{
    apply_idle_airflow();
}
