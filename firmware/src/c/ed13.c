#include "stock_registers.h"

static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start;
static __sfr __at (0xdc) adc_channel;
static __sfr __at (0xf0) B;
static __sbit __at (0xdc) adc_busy;

void read_monitor_adc_high(void)
{
    adc_channel = B;
    adc_channel |= 0x40;
    adc_start = 0;
    while (adc_busy) {
    }
    ACC = adc_result_high;
}
