#include "stock_registers.h"

enum {
    KNOCK_SELECT = 0x20,
    KNOCK_ATTENUATION_COMMAND = 0xc0,
    KNOCK_ATTENUATION_MASK = 0xdf,
    KNOCK_READ_COMMAND = 0x40,
    KNOCK_FILTER_MASK = 0x3f,
    PERIOD_BASE_INDEX = 0xb0,
};

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sfr __at (0xf9) knock_serial_port;
static __sbit __at (0x53) startup_flag_53;

static volatile __data __at (0x05) uint8_t stock_r5;
static volatile __data __at (0x43) uint8_t knock_mask_index;
static volatile __data __at (0x47) uint8_t knock_attenuation;
static volatile __data __at (0x7b) uint8_t knock_window_override;
static volatile __xdata __at (0xf93b) uint8_t knock_filter_point_raw;
static volatile __xdata __at (0xf9c7) uint8_t crank_period_buffer[14];
static volatile __xdata __at (0xf9d5) uint8_t crank_period_index;
static volatile __xdata __at (0xfa17) uint16_t misfire_bounded_word;
static volatile __xdata __at (0xfa19) uint8_t scaled_period_buffer[14];

extern void send_knock_command(void);
extern void wait_knock_filter(void);
extern void continue_misfire_period_history(void);
extern void skip_misfire_period_scaling(void);

#define PERIOD_UNITY_Q15 UINT16_C(0x8000)

#define SEND_KNOCK_COMMAND()                        \
    do {                                            \
        knock_serial_port &= ~KNOCK_SELECT;          \
        send_knock_command();                       \
        knock_serial_port |= KNOCK_SELECT;           \
    } while (0)

void scale_misfire_period_sample(void);

void finish_knock_sample(void)
{
    // The stock branch on this read joins immediately at its target.
    ACC = knock_window_override & 0x10;
    B = (knock_attenuation | KNOCK_ATTENUATION_COMMAND) & KNOCK_ATTENUATION_MASK;
    SEND_KNOCK_COMMAND();
    wait_knock_filter();

    B = (knock_filter_point_raw & 0x7f) | KNOCK_READ_COMMAND;
    SEND_KNOCK_COMMAND();
    wait_knock_filter();

    B = knock_filter_point_raw & KNOCK_FILTER_MASK;
    SEND_KNOCK_COMMAND();
    if (startup_flag_53) {
        skip_misfire_period_scaling();
        return;
    }
    scale_misfire_period_sample();
}

void scale_misfire_period_sample(void)
{
    stock_r5 = crank_period_index - PERIOD_BASE_INDEX;
    stock_r01 = *(volatile __xdata uint16_t *)((uint16_t)crank_period_buffer + stock_r5);
    stock_r23 = (knock_mask_index & 1) ? PERIOD_UNITY_Q15 : misfire_bounded_word;

    // Interleaved operands and four settling instructions are part of the MDU interface.
    mdu_0 = stock_r2;
    mdu_4 = stock_r0;
    mdu_1 = stock_r3;
    mdu_5 = stock_r1;
    __asm
        nop
        nop
        nop
        nop
    __endasm;

    stock_r2 = mdu_0;
    ACC = mdu_1;
    stock_r0 = mdu_2;
    stock_r1 = mdu_3;
    stock_r01 = (stock_r01 << 1) | (ACC >> 7);
    *(volatile __xdata uint16_t *)((uint16_t)scaled_period_buffer + stock_r5) = stock_r01;
    continue_misfire_period_history();
}
