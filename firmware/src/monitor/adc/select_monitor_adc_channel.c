#include "stock_registers.h"

static __sfr __at (0x89) TMOD;
static __sfr __at (0x8a) TL0;
static __sfr __at (0x8c) TH0;
static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start_result_low;
static __sfr __at (0xdc) adc_channel;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x05) monitor_request_invalid;
static __sbit __at (0x8c) timer0_running;
static __sbit __at (0x8d) timer0_overflow;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xdc) adc_busy;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x3f) uint8_t monitor_request_kind;
static volatile __xdata __at (0xf800) uint8_t monitor_adc_samples[512];

extern void resume_service_monitor(void);

void select_monitor_adc_channel(void)
{
    do {
        stock_r0 = *(const __code uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        stock_r4 = *(const __code uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        if (stock_r0 == 0xff) {
            monitor_command_active = 0;
            monitor_request_invalid = 1;
            resume_service_monitor();
            return;
        }
    } while (stock_r0 != monitor_request_kind);
}

#define RESTART_SAMPLE_TIMER()                       \
    do {                                             \
        timer0_running = 0;                          \
        TL0 = 0x66;                                  \
        TH0 = 0xfd;                                  \
        timer0_overflow = 0;                         \
        timer0_running = 1;                          \
    } while (0)

void capture_monitor_adc_samples(void)
{
    TMOD = 1;
    RESTART_SAMPLE_TIMER();
    STOCK_DPTR = (uint16_t)monitor_adc_samples;
    B = 0;
    do {
        while (!timer0_overflow) {
        }
        adc_channel = stock_r4;
        adc_channel |= 0x40;
        adc_start_result_low = 0;
        while (adc_busy) {
        }
        stock_r0 = adc_result_high;
        ACC = adc_start_result_low;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;
        ACC = stock_r0;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;

        RESTART_SAMPLE_TIMER();
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
    } while (--B);
}
