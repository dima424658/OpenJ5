#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0x10) maf_baseline_fault;
static __sbit __at (0x16) retained_ram_checksum_failed;
static __sbit __at (0x36) inhibit_maf_filter_36;
static __sbit __at (0x53) maf_filter_condition_53;
static __sbit __at (0x6b) maf_filter_condition_6b;
static volatile __xdata __at (0xf4e0) uint8_t diagnostic_f4e0;
static volatile __xdata __at (0xf4e8) uint8_t diagnostic_f4e8;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xf9a4) uint8_t maf_baseline;
static volatile __xdata __at (0xf9a5) uint8_t maf_crank_threshold;
static volatile __xdata __at (0xf9a6) uint8_t maf_filter_high;
static volatile __xdata __at (0xf9a7) uint8_t maf_filter_low;
static volatile __xdata __at (0xf9a8) uint8_t maf_above_threshold_count;

extern __code const uint8_t maf_baseline_fault_delay;
extern void read_adc_8bit(void);
extern void filter_maf_baseline(void);
extern void sample_adc10(void);

void reset_maf_baseline_count(void);

void continue_after_sample_batch(void)
{
    if (!maf_filter_condition_53 || inhibit_maf_filter_36 ||
            retained_ram_checksum_failed)
        goto reset_count;

    ACC = retained_status;
    if (ACC_BIT1 || !maf_filter_condition_6b || diagnostic_f4e0 != 0 ||
            diagnostic_f4e8 != 0 || maf_baseline == 0)
        goto reset_count;

    stock_r2 = maf_filter_high;
    stock_r1 = maf_filter_low;
    B = 0;
    read_adc_8bit();
    stock_r3 = ACC;
    ACC = maf_baseline;
    const uint8_t sample = stock_r3;
    stock_r3 = ACC;
    ACC = sample;
    filter_maf_baseline();
    stock_r3 = ACC;
    maf_baseline = ACC;
    maf_filter_high = stock_r2;
    maf_filter_low = stock_r1;

    if (maf_crank_threshold < stock_r3) {
        ACC = maf_above_threshold_count + 1;
        maf_above_threshold_count = ACC;
        stock_r3 = ACC;
        if (maf_baseline_fault_delay >= stock_r3) {
            sample_adc10();
            return;
        }
        maf_baseline_fault = 1;
    }

reset_count:
    reset_maf_baseline_count();
}

void reset_maf_baseline_count(void)
{
    maf_above_threshold_count = 0;
    sample_adc10();
}
