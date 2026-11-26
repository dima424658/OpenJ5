#include "stock_registers.h"

enum {
    MAF_ADC_CHANNEL = 0,
    MAF_BASELINE_MIN = 40,
    MAF_BASELINE_MAX_EXCLUSIVE = 60,
};

static __sfr __at (0xf0) B;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __xdata __at (0xf94c) int8_t poststart_iac_offset;
static volatile __xdata __at (0xf9a4) uint8_t maf_baseline;
static volatile __xdata __at (0xf9a5) uint8_t maf_crank_threshold;
static volatile __xdata __at (0xf9a6) uint8_t startup_state_f9a6;
static volatile __xdata __at (0xf9a7) uint8_t startup_state_f9a7;
static volatile __xdata __at (0xf9a8) uint8_t startup_state_f9a8;

extern volatile __code const uint8_t maf_crank_threshold_offset;
extern __code const int8_t poststart_iac_upper_offset_8c00[39];

extern void read_adc_8bit(void);
extern void continue_startup_sensor_mode(void);
void init_startup_iac_offset(void);

void continue_startup_sensor_setup(void)
{
    ACC = 0;
    maf_baseline = ACC;
    maf_crank_threshold = ACC;
    startup_state_f9a8 = ACC;
    startup_state_f9a6 = 3;
    startup_state_f9a7 = 0x2e;

    B = MAF_ADC_CHANNEL;
    read_adc_8bit();
    stock_r0 = ACC;
    if (ACC >= MAF_BASELINE_MIN && ACC < MAF_BASELINE_MAX_EXCLUSIVE) {
        maf_baseline = stock_r0;
        maf_crank_threshold = (uint8_t)(maf_crank_threshold_offset + stock_r0);
    }
    init_startup_iac_offset();
}

void init_startup_iac_offset(void)
{
    poststart_iac_offset = poststart_iac_upper_offset_8c00[coolant_nearest_index];
    continue_startup_sensor_mode();
}
