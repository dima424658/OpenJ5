#include "monitor_adc.h"
#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;

static volatile __data __at (0x44) uint8_t monitor_adc_minimum;
static volatile __data __at (0x45) uint8_t monitor_adc_maximum;
static volatile __data __at (0x46) uint8_t monitor_adc_mean_low;
static volatile __data __at (0x47) uint8_t monitor_adc_mean_high;
static volatile __data __at (0x48) uint8_t monitor_adc_span;
static volatile __xdata __at (0xf800) uint8_t monitor_adc_samples[512];

extern void select_monitor_adc_channel(void);
extern void capture_monitor_adc_samples(void);
extern void resume_service_monitor(void);

void summarize_monitor_adc_samples(void)
{
    STOCK_DPTR = (uint16_t)monitor_adc_samples;
    B = 0;
    stock_r01 = 0;
    stock_r2 = 0;
    do {
        const uint8_t low = *(volatile __xdata uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        const uint8_t high = *(volatile __xdata uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        const uint16_t prior = stock_r01;
        stock_r01 += ((uint16_t)high << 8) | low;
        if (stock_r01 < prior) {
            ++stock_r2;
        }
    } while (--B);
    monitor_adc_mean_low = stock_r1;
    monitor_adc_mean_high = stock_r2;

    STOCK_DPTR = (uint16_t)monitor_adc_samples;
    B = 255;
    ++STOCK_DPTR;
    ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
    ++STOCK_DPTR;
    stock_r0 = ACC;
    stock_r1 = ACC;

    do {
        ++STOCK_DPTR;
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        if (ACC < stock_r0) {
            stock_r0 = ACC;
        }
        if (ACC >= stock_r1) {
            stock_r1 = ACC;
        }
    } while (--B);
    monitor_adc_minimum = stock_r0;
    monitor_adc_maximum = stock_r1;
    ACC = stock_r1 - stock_r0;
    monitor_adc_span = ACC;
}

void sample_monitor_adc(void)
{
    STOCK_DPTR = (uint16_t)monitor_adc_channels;
    select_monitor_adc_channel();
    capture_monitor_adc_samples();
    summarize_monitor_adc_samples();
    monitor_command_active = 0;
    monitor_result_ready = 1;
    resume_service_monitor();
}
