#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x09) throttle_zero_adaptation_enabled;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x4b) sensor_batch_ready;
static volatile __xdata __at (0xf805) uint8_t throttle_zero_drift_timer;
static volatile __xdata __at (0xf81d) uint16_t throttle_sample_sum;
static volatile __xdata __at (0xf81f) uint16_t rpm_sample_sum;
static volatile __xdata __at (0xf821) uint16_t previous_rpm_sum;
static volatile __xdata __at (0xf821) uint8_t previous_rpm_low;
static volatile __xdata __at (0xf822) uint8_t previous_rpm_high;
static volatile __xdata __at (0xf823) uint8_t rpm_sum_increase;
static volatile __xdata __at (0xf825) uint8_t throttle_sample_minimum;
static volatile __xdata __at (0xf826) uint8_t throttle_sample_maximum;
static volatile __xdata __at (0xf827) uint8_t throttle_zero_reference;
static volatile __xdata __at (0xf8b2) uint8_t throttle_position_baseline;

extern __code const uint8_t throttle_adaptation_band;
extern volatile __code const uint8_t startup_throttle_zero;
extern __code const uint8_t throttle_adc_limits[2];
extern void shift_left_saturating_3(void);
extern void subtract_u16_register_pairs(void);
extern void clear_sensor_sample_sums(void);
extern void continue_after_sample_batch(void);

void drift_throttle_baseline(void);
void diagnose_throttle_samples(void);
void publish_rpm_sample_increase(void);

void update_throttle_zero_samples(void)
{
    B = throttle_sample_minimum;
    B = throttle_sample_maximum - B;
    if (throttle_adaptation_band >= B) {
        stock_r01 = throttle_sample_sum;
        shift_left_saturating_3();
        if (throttle_position_baseline >= stock_r1) {
            ACC = stock_r1;
            throttle_zero_reference = ACC;
            throttle_position_baseline = ACC;
            diagnose_throttle_samples();
            return;
        }
    }
    drift_throttle_baseline();
}

void drift_throttle_baseline(void)
{
    if (throttle_zero_adaptation_enabled && throttle_zero_drift_timer == 0) {
        B = startup_throttle_zero;
        ACC = throttle_position_baseline;
        if (ACC < B)
            throttle_position_baseline = ACC + 1;
    }
    diagnose_throttle_samples();
}

void diagnose_throttle_samples(void)
{
    stock_r01 = throttle_sample_sum;
    shift_left_saturating_3();
    B = throttle_adc_limits[0];
    if (stock_r1 < B) {
        throttle_adc_low = 1;
        throttle_adc_high = 0;
    } else {
        B = throttle_adc_limits[1];
        if (stock_r1 >= B) {
            throttle_adc_low = 0;
            throttle_adc_high = 1;
        } else {
            throttle_adc_low = 0;
            throttle_adc_high = 0;
        }
    }

    publish_rpm_sample_increase();
}

void publish_rpm_sample_increase(void)
{
    stock_r01 = rpm_sample_sum;
    stock_r23 = previous_rpm_sum;
    previous_rpm_high = stock_r1;
    previous_rpm_low = stock_r0;
    subtract_u16_register_pairs();
    if (CY)
        stock_r01 = 0;
    if (stock_r1 != 0)
        stock_r0 = UINT8_MAX;
    rpm_sum_increase = stock_r0;

    clear_sensor_sample_sums();
    sensor_batch_ready = 1;
    continue_after_sample_batch();
}
