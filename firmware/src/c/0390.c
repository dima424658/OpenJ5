#include <stdint.h>

static __sfr __at (0xa1) spark_compare_low;
static __sfr __at (0xa2) spark_compare_high;
static __sfr __at (0xa3) dwell_compare_low;
static __sfr __at (0xa4) dwell_compare_high;
static __sfr __at (0xa5) spark_compare_control;
static __sfr __at (0xa6) dwell_compare_control;
static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start;
static __sfr __at (0xdc) adc_channel;

static __sbit __at (0x3d) second_ignition_coil;
static __sbit __at (0x43) adc_interrupted_by_iac;
static __sbit __at (0x52) primary_spark_pending;
static __sbit __at (0x94) knock_latch;
static __sbit __at (0xdc) adc_busy;
static __sbit __at (0xfd) tooth_marker_output;

static volatile __data __at (0x10) uint8_t coil_1_dwell_tooth;
static volatile __data __at (0x11) uint8_t coil_1_dwell_fraction;
static volatile __data __at (0x12) uint8_t coil_1_spark_tooth;
static volatile __data __at (0x13) uint8_t coil_1_spark_fraction;
static volatile __data __at (0x14) uint8_t coil_2_dwell_tooth;
static volatile __data __at (0x15) uint8_t coil_2_dwell_fraction;
static volatile __data __at (0x16) uint8_t coil_2_spark_tooth;
static volatile __data __at (0x17) uint8_t coil_2_spark_fraction;
static volatile __data __at (0x1a) uint16_t last_crank_capture;
static volatile __data __at (0x1c) uint16_t previous_tooth_period;
static volatile __data __at (0x1e) uint8_t crank_tooth;
static volatile __data __at (0x1f) uint8_t half_turn_tooth;
static volatile __data __at (0x42) uint8_t knock_noise_floor;
static volatile __data __at (0x4c) uint8_t knock_begin_pair_1;
static volatile __data __at (0x72) uint8_t knock_adc_sample;
static volatile __data __at (0x74) uint8_t knock_end_pair_1;
static volatile __data __at (0x76) uint8_t knock_adc_raw;

void irq_crank_epilogue(void);
void irq_crank_reti(void);

#define IGNITION_COMPARE_TICK(fraction)                                              \
    (uint16_t)(last_crank_capture +                                                  \
               (((uint32_t)(uint16_t)(previous_tooth_period << 1) * (fraction)) >> 8))

void update_crank_angular_outputs(void)
{
    if (half_turn_tooth == knock_begin_pair_1) {
        knock_latch = 1;
    }
    if (half_turn_tooth == knock_end_pair_1) {
        knock_latch = 0;
    }
    if (half_turn_tooth == 6) {
        adc_interrupted_by_iac = 1;
        adc_channel = 7;
        adc_start = 0;
        while (adc_busy) {
        }
        const uint8_t sample = adc_result_high;
        knock_adc_raw = sample;
        knock_adc_sample = sample < knock_noise_floor ? 0 : sample - knock_noise_floor;
    }
    if (half_turn_tooth == 11) {
        tooth_marker_output = 0;
    }
    if (half_turn_tooth == 1) {
        tooth_marker_output = 1;
    }

    if (crank_tooth == coil_1_dwell_tooth) {
        const uint16_t compare = IGNITION_COMPARE_TICK(coil_1_dwell_fraction);
        dwell_compare_control &= 0xf9;
        spark_compare_control &= 0xfd;
        dwell_compare_high = compare >> 8;
        dwell_compare_low = (uint8_t)compare;
        dwell_compare_control |= 0x02;
    }
    if (crank_tooth == coil_1_spark_tooth) {
        primary_spark_pending = 1;
        const uint16_t compare = IGNITION_COMPARE_TICK(coil_1_spark_fraction);
        spark_compare_control &= 0xf9;
        dwell_compare_control &= 0xfd;
        spark_compare_high = compare >> 8;
        spark_compare_low = (uint8_t)compare;
        spark_compare_control |= 0x02;
        second_ignition_coil = 0;
    }
    if (crank_tooth == coil_2_dwell_tooth) {
        const uint16_t compare = IGNITION_COMPARE_TICK(coil_2_dwell_fraction);
        dwell_compare_control &= 0xf9;
        spark_compare_control &= 0xfb;
        dwell_compare_high = compare >> 8;
        dwell_compare_low = (uint8_t)compare;
        dwell_compare_control |= 0x04;
    }
    if (crank_tooth == coil_2_spark_tooth) {
        primary_spark_pending = 1;
        const uint16_t compare = IGNITION_COMPARE_TICK(coil_2_spark_fraction);
        spark_compare_control &= 0xf9;
        dwell_compare_control &= 0xfb;
        spark_compare_high = compare >> 8;
        spark_compare_low = (uint8_t)compare;
        spark_compare_control |= 0x04;
        second_ignition_coil = 1;
    }
    irq_crank_epilogue();
}

void irq_crank_epilogue(void) __naked
{
    __asm
        pop b
        pop acc
        pop psw
    __endasm;
    irq_crank_reti();
}

void irq_crank_reti(void) __naked
{
    __asm reti __endasm;
}
