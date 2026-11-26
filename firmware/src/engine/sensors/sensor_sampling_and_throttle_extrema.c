#include "stock_registers.h"

enum {
    THROTTLE_ADC_CHANNEL = 3,
    SENSOR_BATCH_SIZE = 32,
    SKIP_COOLANT_UPDATE = 0x80,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x7a) uint8_t sensor_mode_flags_7a;
static volatile __xdata __at (0xf812) uint8_t sensor_sample_count;
static volatile __xdata __at (0xf81d) uint16_t throttle_sample_sum;
static volatile __xdata __at (0xf81f) uint16_t rpm_sample_sum;
static volatile __xdata __at (0xf825) uint8_t throttle_sample_minimum;
static volatile __xdata __at (0xf826) uint8_t throttle_sample_maximum;
static volatile __xdata __at (0xf8a9) uint8_t throttle_adc_raw;

extern void read_adc_8bit(void);
extern void accumulate_u8_xdata_u16(void);
extern void update_coolant_axis(void);
extern void update_coolant_from_samples(void);
extern void continue_after_sample_batch(void);

void track_throttle_extrema(void);
void finish_sensor_sample(void);

void continue_sampling_sensors(void)
{
    B = THROTTLE_ADC_CHANNEL;
    read_adc_8bit();
    throttle_adc_raw = ACC;
    STOCK_DPTR = (uint16_t)&throttle_sample_sum;
    accumulate_u8_xdata_u16();
    track_throttle_extrema();
}

void track_throttle_extrema(void)
{
    /* The accumulator routine leaves the most recent reading in B. */
    if (sensor_sample_count == 0) {
        throttle_sample_minimum = B;
        throttle_sample_maximum = B;
    } else if (throttle_sample_minimum >= B) {
        throttle_sample_minimum = B;
    } else if (throttle_sample_maximum < B) {
        throttle_sample_maximum = B;
    }

    finish_sensor_sample();
}

void finish_sensor_sample(void)
{
    ACC = rpm_eighth;
    STOCK_DPTR = (uint16_t)&rpm_sample_sum;
    accumulate_u8_xdata_u16();

    ACC = sensor_sample_count + 1;
    sensor_sample_count = ACC;
    if (ACC != SENSOR_BATCH_SIZE) {
        continue_after_sample_batch();
        return;
    }

    sensor_sample_count = 0;
    if ((sensor_mode_flags_7a & SKIP_COOLANT_UPDATE) != 0) {
        update_coolant_axis();
        return;
    }
    update_coolant_from_samples();
}
