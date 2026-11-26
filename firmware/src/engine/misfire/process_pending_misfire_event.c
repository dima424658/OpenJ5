#include "stock_registers.h"

enum { SAMPLE_ACCEPTED = 0x02 };

static __sbit __at (0xd7) CY;
static __sbit __at (0x5c) fuel_mode_flag_5c;

static volatile __xdata __at (0xfa63) uint16_t misfire_samples[4];
static volatile __xdata __at (0xfa6c) uint8_t misfire_control_fa6c;
static volatile __xdata __at (0xfa6d) uint8_t misfire_control_fa6d;
static volatile __xdata __at (0xfa6e) uint16_t misfire_raw_event;
static volatile __xdata __at (0xfa70) uint16_t misfire_scaled_event;
static volatile __xdata __at (0xfa72) uint8_t misfire_counters[4];

extern __code const uint8_t misfire_event_limits[4];
extern void shift_right_u16_2(void);
extern void subtract_u16_register_pairs(void);
extern void compare_misfire_samples(void);
extern void publish_misfire_counter_flags(void);

void count_misfire_samples(void);

void process_pending_misfire_event(void)
{
    stock_r01 = misfire_raw_event;
    shift_right_u16_2();
    misfire_scaled_event = stock_r01;

    stock_r01 = misfire_raw_event;
    stock_r2 = misfire_event_limits[0];
    stock_r3 = misfire_event_limits[1];
    subtract_u16_register_pairs();
    if (!CY) {
        misfire_control_fa6d |= SAMPLE_ACCEPTED;
        fuel_mode_flag_5c = 0;
        count_misfire_samples();
        return;
    }

    misfire_control_fa6d &= (uint8_t)~SAMPLE_ACCEPTED;
    fuel_mode_flag_5c = 1;
    ACC = misfire_counters[0];
    if (ACC != 0)
        misfire_counters[0] = ACC - 1;
    ACC = misfire_counters[1];
    if (ACC != 0)
        misfire_counters[1] = ACC - 1;
    ACC = misfire_counters[2];
    if (ACC != 0)
        misfire_counters[2] = ACC - 1;
    ACC = misfire_counters[3];
    if (ACC != 0)
        misfire_counters[3] = ACC - 1;

    misfire_control_fa6c &= 0x1f;
    misfire_control_fa6d &= 0xfe;
    publish_misfire_counter_flags();
}

void count_misfire_samples(void)
{
    stock_r01 = misfire_samples[0];
    stock_r2 = misfire_event_limits[2];
    stock_r3 = misfire_event_limits[3];
    subtract_u16_register_pairs();
    if (!CY) {
        ACC = misfire_counters[0] + 1;
        if (ACC != 0)
            misfire_counters[0] = ACC;
    }

    stock_r01 = misfire_samples[1];
    subtract_u16_register_pairs();
    if (!CY) {
        ACC = misfire_counters[1] + 1;
        if (ACC != 0)
            misfire_counters[1] = ACC;
    }

    stock_r01 = misfire_samples[2];
    subtract_u16_register_pairs();
    if (!CY) {
        ACC = misfire_counters[2] + 1;
        if (ACC != 0)
            misfire_counters[2] = ACC;
    }

    stock_r01 = misfire_samples[3];
    subtract_u16_register_pairs();
    if (!CY) {
        ACC = misfire_counters[3] + 1;
        if (ACC != 0)
            misfire_counters[3] = ACC;
    }
    compare_misfire_samples();
}
