#include <stdint.h>

static __sfr __at (0xa5) sfr_a5;
static __sfr __at (0xa6) sfr_a6;
static __sfr __at (0xe0) ACC;
static __sfr __at (0xe8) sfr_e8;
static __sfr __at (0xf2) sfr_f2;
static __sfr __at (0xf3) sfr_f3;
static __sfr __at (0xf6) sfr_f6;

static __sbit __at (0x31) event_flag_31;
static __sbit __at (0xbd) event_irq_high_priority;
static __sbit __at (0xf9) output_control_f9;
static __sbit __at (0xfa) output_control_fa;
static __sbit __at (0xfe) output_control_fe;

static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x54) uint8_t rpm_state_54;
static volatile __data __at (0x55) uint8_t rpm_state_55;
static volatile __data __at (0x57) uint8_t rpm_axis_index_q4;
static volatile __data __at (0x58) uint8_t rpm_half_step_index;
static volatile __data __at (0x7c) uint8_t output_flags_7c;
static volatile __xdata __at (0xf806) uint8_t state_f806;
static volatile __xdata __at (0xf999) uint8_t relay_outputs;

void stop_engine_outputs(void)
{
    event_irq_high_priority = 0;
    relay_outputs &= 0x9f;
    ACC = 0;
    state_f806 = ACC;
    sfr_f3 = ACC;
    sfr_f2 = 0x0f;
    sfr_f6 &= 0xf0;
    sfr_e8 = 0x0f;
    output_flags_7c = 0;
    output_control_fe = 1;
    sfr_a5 |= 0x06;
    output_control_f9 = 1;
    output_control_fa = 1;
    sfr_a6 &= 0xf9;

    engine_rpm_10 = 0;
    rpm_state_54 = 0;
    rpm_state_55 = 0;
    rpm_axis_index_q4 = 0;
    rpm_half_step_index = 0;
    event_flag_31 = 0;
}
