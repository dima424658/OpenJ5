#include "monitor_adc.h"
#include "monitor_port9_config.h"
#include "stock_registers.h"

static __sfr __at (0x89) TMOD;
static __sfr __at (0x8a) TL0;
static __sfr __at (0x8c) TH0;
static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start_result_low;
static __sfr __at (0xdc) adc_channel;
static __sfr __at (0xf0) B;
static __sfr __at (0xf9) P9;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x8c) timer0_running;
static __sbit __at (0x8d) timer0_overflow;
static __sbit __at (0x94) measurement_window;
static __sbit __at (0xd7) CY;
static __sbit __at (0xdc) adc_busy;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x44) uint8_t monitor_adc_minimum;
static volatile __data __at (0x45) uint8_t monitor_adc_maximum;
static volatile __data __at (0x46) uint8_t monitor_adc_mean_low;
static volatile __data __at (0x47) uint8_t monitor_adc_mean_high;
static volatile __data __at (0x48) uint8_t monitor_adc_span;

extern void select_monitor_adc_channel(void);
extern void poll_monitor_exit_and_io(void);
extern void resume_service_monitor(void);

void measure_monitor_adc_window(void)
{
    STOCK_DPTR = (uint16_t)monitor_window_adc_channels;
    select_monitor_adc_channel();

    ACC = 0x2c;
    SEND_PORT9_CONFIG();
    CONFIG_DELAY_32();

    ACC = 0x6c;
    SEND_PORT9_CONFIG();
    CONFIG_DELAY_32();

    ACC = 0x82;
    SEND_PORT9_CONFIG();
    CONFIG_DELAY_32();

    ACC = 0xdf;
    SEND_PORT9_CONFIG();
    CONFIG_DELAY_32();

    ACC = 0xe1 | ((stock_r4 & 1) << 1);
    SEND_PORT9_CONFIG();
    CONFIG_DELAY_32();

    measurement_window = 1;
    TMOD = 1;
    timer0_running = 0;
    TL0 = 0xeb;
    TH0 = 0xcb;
    timer0_overflow = 0;
    timer0_running = 1;
    while (!timer0_overflow) {
    }
    measurement_window = 0;
    CONFIG_DELAY_32();

    adc_channel = 7;
    adc_channel |= 0x40;
    adc_start_result_low = 0;
    while (adc_busy) {
    }
    ACC = adc_result_high;
    B = adc_start_result_low;
    monitor_adc_mean_low = B;
    monitor_adc_mean_high = ACC;
    monitor_adc_minimum = ACC;
    monitor_adc_maximum = ACC;
    monitor_adc_span = 0;
    poll_monitor_exit_and_io();
    monitor_command_active = 0;
    monitor_result_ready = 1;
    resume_service_monitor();
}
