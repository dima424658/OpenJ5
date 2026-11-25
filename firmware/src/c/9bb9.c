#include <stdint.h>

static __sfr __at (0xd9) adc_result_high;
static __sfr __at (0xda) adc_start;
static __sfr __at (0xdc) adc_channel;
static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;

static __sbit __at (0xdc) adc_busy;
static __sbit __at (0x43) adc_interrupted_by_iac;

void read_adc_8bit(void)
{
    adc_interrupted_by_iac = 0;
    do {
        adc_channel = B;
        adc_start = 0;
        while (adc_busy) {
        }
        ACC = adc_result_high;

        /* Restart the conversion if the IAC interrupt used the ADC. */
        if (!adc_interrupted_by_iac) {
            break;
        }
        adc_interrupted_by_iac = 0;
    } while (1);
}
