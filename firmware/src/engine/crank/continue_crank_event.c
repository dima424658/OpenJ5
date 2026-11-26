#include "stock_registers.h"

enum { MAF_ZERO_OFFSET = 1000, MAF_LOW_ADC_LIMIT = 7 };

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;

static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x26) maf_adc_high;
static __sbit __at (0x3f) throttle_moved;
static __sbit __at (0x40) crank_airflow_pending;
static __sbit __at (0x43) adc_interrupted_by_iac;
static __sbit __at (0x45) output_state_flag_45;
static __sbit __at (0x4c) ignition_supply_low;
static __sbit __at (0x53) startup_flag_53;

static volatile __data __at (0x33) uint8_t maf_sample_count;
static volatile __data __at (0x34) uint8_t maf_sample_sum[2];
static volatile __data __at (0x41) uint8_t crank_state_41;
static volatile __xdata __at (0xf808) uint16_t mass_airflow;
static volatile __xdata __at (0xf80c) uint8_t maf_adc_sample;
static volatile __xdata __at (0xf827) uint8_t throttle_zero_reference;
static volatile __xdata __at (0xf949) uint8_t charge_filter_coefficient;
static volatile __xdata __at (0xf989) uint8_t throttle_stable_count;
static volatile __xdata __at (0xf98c) uint8_t crank_throttle_position;
static volatile __xdata __at (0xfa61) uint8_t crank_state_snapshot;

extern volatile __code const uint8_t maf_min_diagnostic_flow;
extern volatile __code const uint8_t throttle_transfer_gain;
extern volatile __code const uint8_t throttle_max_position;
extern volatile __code const uint8_t dynamic_charge_filter_gain;
extern volatile __code const uint8_t throttle_stable_limit;
extern volatile __code const uint8_t stable_charge_filter_gain;

extern void finish_crank_interrupt(void);
extern void read_adc_8bit(void);
extern void abs_accumulator_i8(void);
extern void continue_airflow_calculation(void);

void update_crank_throttle_gain(void);

void continue_crank_event(void)
{
    if (!crank_airflow_pending) {
        finish_crank_interrupt();
        return;
    }
    crank_airflow_pending = 0;
    crank_state_snapshot = crank_state_41;
    if (maf_sample_count == 0) {
        update_crank_throttle_gain();
        return;
    }

    mdu_0 = maf_sample_sum[0];
    mdu_1 = maf_sample_sum[1];
    mdu_4 = maf_sample_count;
    mdu_5 = 0;
    __asm
        nop
        nop
        nop
        nop
    __endasm;
    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    (void)mdu_4;
    (void)mdu_5;

    if (ignition_supply_low) {
        goto zero_airflow;
    }
    stock_r01 = stock_r01 < MAF_ZERO_OFFSET ? 0 : stock_r01 - MAF_ZERO_OFFSET;
    if (stock_r1 == 0 && !startup_flag_53 && stock_r0 < maf_min_diagnostic_flow) {
        maf_adc_low = maf_adc_sample < MAF_LOW_ADC_LIMIT;
        maf_adc_high = 0;
        goto zero_airflow;
    }

    maf_adc_low = 0;
    mass_airflow = stock_r01;
    update_crank_throttle_gain();
    return;

zero_airflow:
    stock_r01 = 0;
    mass_airflow = stock_r01;
    update_crank_throttle_gain();
}

void update_crank_throttle_gain(void)
{
    if (output_state_flag_45) {
        charge_filter_coefficient = dynamic_charge_filter_gain;
        throttle_moved = 0;
        throttle_stable_count = 0;
        continue_airflow_calculation();
        return;
    }

    if (!throttle_adc_low && !throttle_adc_high) {
        stock_r2 = crank_throttle_position;
        B = 3;
        read_adc_8bit();
        B = ACC;
        adc_interrupted_by_iac = 1;
        if (B < throttle_zero_reference) {
            continue_airflow_calculation();
            return;
        }
        B -= throttle_zero_reference;
        B = (uint16_t)B * throttle_transfer_gain >> 8;
        crank_throttle_position = B;
        if (B > throttle_max_position) {
            crank_throttle_position = throttle_max_position;
        }
    }

    // The ADC fault path keeps the R2 reference supplied by the caller.
    ACC = crank_throttle_position - stock_r2;
    abs_accumulator_i8();
    if (ACC >= 2) {
        throttle_moved = 1;
        throttle_stable_count = 0;
        charge_filter_coefficient = dynamic_charge_filter_gain;
    } else {
        ACC = throttle_stable_count + 1;
        if (ACC >= throttle_stable_limit) {
            throttle_moved = 0;
            charge_filter_coefficient = stable_charge_filter_gain;
        } else {
            throttle_stable_count = ACC;
        }
    }
    continue_airflow_calculation();
}
