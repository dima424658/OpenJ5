#include "stock_registers.h"

enum {
    KNOCK_SELECT = 0x20,
    KNOCK_POINT_COMMAND = 0x80,
    KNOCK_POINT_MASK = 0xbf,
    MIN_NOISE_FOR_SAMPLE_LIMIT = 10,
};

static __sfr __at (0xf0) B;
static __sfr __at (0xf9) knock_control_port;
static __sbit __at (0x42) knock_sample_pending;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x08) uint8_t bank_one_noise_floor;
static volatile __data __at (0x43) uint8_t knock_mask_index;
static volatile __data __at (0x54) uint8_t rpm_state_54;
static volatile __data __at (0x72) uint8_t knock_adc_sample;
static volatile __idata __at (0xa0) uint8_t cylinder_raw_samples[4];
static volatile __xdata __at (0xf943) uint8_t knock_threshold_multiplier_q4;
static volatile __xdata __at (0xf966) uint16_t cylinder_noise_accumulator[4];
static volatile __xdata __at (0xf96e) uint8_t cylinder_filtered_noise[4];
static volatile __xdata __at (0xf972) uint8_t cylinder_knock_threshold[4];
static volatile __xdata __at (0xfa60) uint8_t knock_sample_cylinder;
static volatile __xdata __at (0xffce) uint8_t retained_knock_noise;

extern __code const uint8_t knock_attenuator_switch_point[256];
extern void send_knock_command(void);
extern void filter_maf_baseline(void);
extern void evaluate_knock_sample(void);

void update_cylinder_knock_threshold(void);

void prepare_knock_sample(void)
{
    knock_sample_pending = 0;
    knock_sample_cylinder = knock_mask_index;
    B = (knock_attenuator_switch_point[rpm_state_54] & KNOCK_POINT_MASK) | KNOCK_POINT_COMMAND;
    knock_control_port &= ~KNOCK_SELECT;
    send_knock_command();
    knock_control_port |= KNOCK_SELECT;
    update_cylinder_knock_threshold();
}

void update_cylinder_knock_threshold(void)
{
    B = knock_adc_sample;
    stock_r4 = B;
    cylinder_raw_samples[knock_mask_index] = B;

    stock_r3 = cylinder_filtered_noise[knock_mask_index];
    stock_r0 = B;
    if (stock_r3 >= MIN_NOISE_FOR_SAMPLE_LIMIT) {
        stock_r0 = B >> 1;
        if (stock_r3 < bank_one_noise_floor) {
            // The ISR runs in bank 1, where this physical write also updates R0.
            bank_one_noise_floor = (uint8_t)(stock_r3 << 1) | (stock_r3 >> 7);
        } else {
            stock_r0 = B;
        }
    }

    const uint16_t sum = cylinder_noise_accumulator[knock_mask_index];
    stock_r1 = (uint8_t)sum;
    stock_r2 = sum >> 8;
    ACC = stock_r0;
    filter_maf_baseline();
    stock_r3 = ACC;
    cylinder_noise_accumulator[knock_mask_index] = ((uint16_t)stock_r2 << 8) | stock_r1;
    cylinder_filtered_noise[knock_mask_index] = stock_r3;

    stock_r1 = B; // The filter preserves the raw sample for the detector.
    B = knock_threshold_multiplier_q4;
    const uint16_t threshold = ((uint16_t)stock_r3 * B) >> 4;
    B = threshold > UINT8_MAX ? UINT8_MAX : threshold;
    ACC = retained_knock_noise;
    if (ACC >= B) {
        B = ACC;
    }
    cylinder_knock_threshold[knock_mask_index] = B;
    stock_r3 = B;
    evaluate_knock_sample();
}
