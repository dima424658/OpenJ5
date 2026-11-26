#include "stock_registers.h"

enum {
    KNOCK_ADC_CHANNEL = 7,
    KNOCK_ADC_BASELINE_LIMIT = 30,
};

static __sfr __at (0xf0) B;
static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start;
static __sfr __at (0xdc) adc_channel;
static __sfr __at (0xf9) knock_serial_port;
static __sbit __at (0xdc) adc_busy;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0x94) knock_latch;
static __sbit __at (0x62) idle_transition_hold;

static volatile __data __at (0x42) uint8_t knock_adc_baseline;
static volatile __xdata __at (0xf8c1) uint8_t idle_transition_counter;

extern volatile __code const uint8_t knock_startup_command_5f9b;
extern volatile __code const uint8_t startup_knock_filter_index;
extern volatile __code const uint8_t knock_startup_command_5f9d;
extern volatile __code const uint8_t knock_startup_adc_command;
extern volatile __code const uint8_t knock_startup_command_5f9f;

extern void send_knock_command(void);
extern void wait_knock_filter(void);
extern void continue_startup_config(void);

/* The caller loads B and waits again when the next operation needs it. */
#define LATCH_KNOCK_COMMAND()       \
    do {                            \
        knock_serial_port &= 0xdf;  \
        send_knock_command();       \
        knock_serial_port |= 0x20;  \
        wait_knock_filter();        \
        knock_latch = 1;            \
    } while (0)

#define PROGRAM_KNOCK_COMMAND(command) \
    do {                               \
        knock_latch = 0;               \
        B = command;                   \
        LATCH_KNOCK_COMMAND();          \
        wait_knock_filter();           \
    } while (0)

void configure_knock_filter(void)
{
    PROGRAM_KNOCK_COMMAND(knock_startup_command_5f9b);
    PROGRAM_KNOCK_COMMAND(startup_knock_filter_index);
    PROGRAM_KNOCK_COMMAND(knock_startup_command_5f9f);
    PROGRAM_KNOCK_COMMAND(knock_startup_command_5f9d);

    knock_latch = 0;
    ACC = knock_startup_adc_command;
    ACC_BIT2 = 1;
    B = ACC;
    LATCH_KNOCK_COMMAND();

    adc_channel = KNOCK_ADC_CHANNEL;
    adc_start = 0;
    while (adc_busy) {
    }
    ACC = adc_result_high;
    if (ACC >= KNOCK_ADC_BASELINE_LIMIT) {
        ACC = KNOCK_ADC_BASELINE_LIMIT;
    }
    knock_adc_baseline = ACC;
    wait_knock_filter();

    knock_latch = 0;
    B = knock_startup_adc_command;
    LATCH_KNOCK_COMMAND();

    idle_transition_hold = 1;
    idle_transition_counter = 1;
    continue_startup_config();
}
