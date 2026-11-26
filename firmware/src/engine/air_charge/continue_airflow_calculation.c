#include "stock_registers.h"

enum { AIRFLOW_PERIOD_DIVISOR = 1000 };

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;

static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x26) maf_adc_high;
static __sbit __at (0xaf) EA;

static volatile __data __at (0x38) uint8_t crank_period[2];
static volatile __data __at (0x48) uint8_t measured_airflow;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __xdata __at (0xf808) uint16_t mass_airflow;
static volatile __xdata __at (0xf864) uint16_t airflow_period_charge;

extern volatile __code const uint8_t maf_max_diagnostic_flow;
extern volatile __code const uint8_t airflow_sensor_fallback;
extern __code const uint8_t reverse_flow_coolant_gain[39];
extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t fallback_cylinder_charge[256];

extern void interp_bilinear_u8(void);
extern void shift_right_u16_3(void);
extern void process_cylinder_charge(void);
extern void continue_charge_after_base(void);

void calculate_airflow_period_charge(void);

void continue_airflow_calculation(void)
{
    stock_r01 = mass_airflow;
    B = maf_max_diagnostic_flow;
    maf_adc_high = stock_r1 != 0 && stock_r1 >= B;
    if (maf_adc_low || maf_adc_high) {
        if (!throttle_adc_low && !throttle_adc_high) {
            stock_r3 = throttle_quantization_q4[throttle_position];
            STOCK_DPTR = (uint16_t)fallback_cylinder_charge;
            stock_r2 = rpm_q4;
            interp_bilinear_u8();
            B = ACC;
            stock_r01 = (uint16_t)reverse_flow_coolant_gain[coolant_nearest_index] * B;
            shift_right_u16_3();
            process_cylinder_charge();
            return;
        }

        B = airflow_sensor_fallback;
        stock_r01 = (uint16_t)reverse_flow_coolant_gain[coolant_nearest_index] * B;
        shift_right_u16_3();
        shift_right_u16_3();
        mass_airflow = stock_r01;
    }

    measured_airflow = stock_r01 < 512 ? stock_r01 >> 1 : UINT8_MAX;
    calculate_airflow_period_charge();
}

void calculate_airflow_period_charge(void)
{
    stock_r01 = mass_airflow;
    EA = 0;
    ACC = crank_period[0];
    B = crank_period[1];
    EA = 1;

    mdu_0 = ACC;
    mdu_4 = stock_r0;
    mdu_1 = B;
    mdu_5 = stock_r1;
    __asm
        nop
        nop
        nop
        nop
    __endasm;
    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    stock_r2 = mdu_2;
    stock_r3 = mdu_3;

    mdu_0 = stock_r0;
    mdu_1 = stock_r1;
    mdu_2 = stock_r2;
    mdu_3 = stock_r3;
    mdu_4 = (uint8_t)AIRFLOW_PERIOD_DIVISOR;
    mdu_5 = AIRFLOW_PERIOD_DIVISOR >> 8;
    __asm
        nop
        nop
        nop
        nop
        nop
        nop
    __endasm;
    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    (void)mdu_2;
    (void)mdu_3;
    (void)mdu_4;
    (void)mdu_5;
    airflow_period_charge = stock_r01;
    continue_charge_after_base();
}
