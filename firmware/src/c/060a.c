#include <stdint.h>

static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start;
static __sfr __at (0xdc) adc_channel;
static __sfr __at (0xfa) stepper_output;

static __sbit __at (0x43) adc_interrupted_by_iac;
static __sbit __at (0x45) inhibit_iac_steps;
static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xdc) adc_busy;

static volatile __data __at (0x30) uint16_t airflow_sample_sum;
static volatile __data __at (0x32) uint8_t airflow_sample_count;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __data __at (0x66) int8_t iac_acceleration;
static volatile __data __at (0x67) uint8_t iac_step_ticks;

static volatile __xdata __at (0xf80c) uint8_t maf_adc_sample;

extern __code const uint8_t iac_acceleration_steps;
extern __code const int8_t iac_phase_direction_offset;
extern __code const uint8_t iac_acceleration_intervals[5];
extern __code const uint8_t maf_transfer_raw[256];
extern __code const uint8_t maf_transfer_high[256];

void sample_timer1_airflow(void);
extern void service_multispark_and_speed(void);

void service_timer1_iac(void)
{
    if (inhibit_iac_steps || (iac_step_ticks && --iac_step_ticks)) {
        goto sample_airflow;
    }

    int16_t distance = (int16_t)iac_target - iac_position;
    if (distance > INT8_MAX) {
        distance = INT8_MAX;
    } else if (distance < INT8_MIN) {
        distance = INT8_MIN;
    }
    if (!distance && iac_acceleration >= -1 && iac_acceleration <= 1) {
        iac_acceleration = 0;
        goto sample_airflow;
    }

    if (iac_acceleration < distance && iac_acceleration != (int8_t)iac_acceleration_steps) {
        if (++iac_acceleration == 0) {
            iac_acceleration = 1;
        }
    } else if (iac_acceleration > distance && iac_acceleration != -(int8_t)iac_acceleration_steps) {
        if (--iac_acceleration == 0) {
            iac_acceleration = -1;
        }
    }

    uint8_t direction_phase = 0;
    if (iac_acceleration < 0) {
        iac_step_ticks = iac_acceleration_intervals[-(int16_t)iac_acceleration];
        --iac_position;
        direction_phase = 2;
    } else {
        iac_step_ticks = iac_acceleration_intervals[iac_acceleration];
        ++iac_position;
    }

    const uint8_t levels = stepper_output;
    const uint8_t phase = (levels & 0x08) | (levels >> 6);
    const uint8_t next = phase + direction_phase + iac_phase_direction_offset;
    stepper_output ^= (next & 2) ? 0x80 : 0x40;

sample_airflow:
    sample_timer1_airflow();
}

void sample_timer1_airflow(void)
{
    uint8_t sample;
    adc_interrupted_by_iac = 0;
    for (;;) {
        adc_channel = 0;
        adc_start = 0;
        while (adc_busy) {
        }
        sample = adc_result_high;
        if (!adc_interrupted_by_iac) {
            break;
        }
        adc_interrupted_by_iac = 0;
    }
    adc_interrupted_by_iac = 1;
    maf_adc_sample = sample;

    /* Publish the sum and count together, dropping samples on word overflow. */
    const uint8_t low = maf_transfer_raw[sample];
    interrupts_enabled = 0;
    const uint16_t airflow = ((uint16_t)maf_transfer_high[sample] << 8) | low;
    const uint16_t next = airflow_sample_sum + airflow;
    if (next >= airflow) {
        airflow_sample_sum = next;
        if (airflow_sample_count != UINT8_MAX) {
            ++airflow_sample_count;
        }
    }
    interrupts_enabled = 1;
    service_multispark_and_speed();
}
