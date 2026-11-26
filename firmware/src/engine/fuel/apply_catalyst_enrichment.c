#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x5c) fuel_mode_flag_5c;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __xdata __at (0xf895) uint8_t exhaust_temperature_high;

extern volatile __code const uint8_t catalyst_protection_temperature;
extern volatile __code const uint8_t catalyst_protection_enrichment;
extern void continue_fuel_mixture_checks(void);

void finish_catalyst_enrichment(void);

void apply_catalyst_enrichment(void)
{
    B = catalyst_protection_temperature;
    if (exhaust_temperature_high >= B) {
        B = catalyst_protection_enrichment;
        const uint8_t mixture = requested_mixture;
        requested_mixture = mixture >= B ? mixture - B : 0;
    }
    finish_catalyst_enrichment();
}

void finish_catalyst_enrichment(void)
{
    fuel_mode_flag_5c = 0;
    continue_fuel_mixture_checks();
}
