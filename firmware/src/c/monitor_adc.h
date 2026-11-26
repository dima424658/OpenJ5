#ifndef JANVAR_MONITOR_ADC_H
#define JANVAR_MONITOR_ADC_H

#include <stdint.h>

struct MonitorAdcChannel {
    uint8_t request;
    uint8_t selector;
};

extern __code const struct MonitorAdcChannel monitor_adc_channels[];

#endif
