#include "stock_registers.h"

static __sfr __at (0xf0) B;
static volatile __data __at (0x47) uint8_t noise_attenuator_state;
static volatile __data __at (0x4f) uint8_t scaled_knock_threshold;
static volatile __xdata __at (0xf931) uint8_t knock_filter_frequency;
static volatile __xdata __at (0xf937) uint8_t knock_threshold;
static volatile __xdata __at (0xf93b) uint8_t knock_filter_index;
static volatile __xdata __at (0xf9aa) uint8_t scaled_knock_noise;
static volatile __xdata __at (0xf9ab) uint8_t knock_noise_index;
static volatile __xdata __at (0xffce) uint8_t retained_knock_noise;

extern volatile __code const uint8_t startup_knock_filter_index;
extern __code const uint8_t knock_filter_frequencies[64];
extern __code const uint8_t knock_relative_threshold_q4[32];

extern void configure_knock_filter(void);
void init_knock_filter_frequency(void);

void continue_startup_attenuation(void)
{
    ACC = (noise_attenuator_state >> 3) & 3;
    if (ACC == 1) {
        ACC = 2;
    } else if (ACC < 1) {
        ACC = 1;
    } else {
        ACC = 4;
    }
    B = ACC;

    uint16_t noise = (uint16_t)retained_knock_noise * B;
    stock_r0 = noise > UINT8_MAX ? UINT8_MAX : (uint8_t)noise;
    ACC = stock_r0;
    scaled_knock_noise = ACC;
    ACC >>= 3;
    knock_noise_index = ACC;

    B = knock_relative_threshold_q4[ACC];
    uint16_t threshold = (uint16_t)retained_knock_noise * B;
    threshold >>= 4;
    ACC = threshold > UINT8_MAX ? UINT8_MAX : (uint8_t)threshold;
    scaled_knock_threshold = ACC;
    knock_threshold = ACC;
    init_knock_filter_frequency();
}

void init_knock_filter_frequency(void)
{
    stock_r1 = startup_knock_filter_index;
    knock_filter_index = stock_r1;
    knock_filter_frequency = knock_filter_frequencies[stock_r1];
    configure_knock_filter();
}
