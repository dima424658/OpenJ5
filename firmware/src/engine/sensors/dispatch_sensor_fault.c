#include <stdint.h>

static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x26) maf_adc_high;

extern void update_sensor_fault_targets(void);
extern void continue_without_sensor_fault(void);

void dispatch_sensor_fault(void)
{
    if (maf_adc_low || maf_adc_high || throttle_adc_low || throttle_adc_high) {
        update_sensor_fault_targets();
    } else {
        continue_without_sensor_fault();
    }
}
