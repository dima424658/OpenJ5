#include "stock_registers.h"

static __sfr __at (0xf0) B;
static volatile __data __at (0x60) uint8_t requested_mixture;

static volatile __xdata __at (0xf832) uint8_t mixture_rate_fraction;
static volatile __xdata __at (0xf833) uint8_t mixture_rate_output;

extern volatile __code const uint8_t mixture_leaning_step_q8;
extern volatile __code const uint8_t mixture_enrichment_step;

void limit_mixture_change_rate(void)
{
    const uint8_t previous = mixture_rate_output;
    if (previous == requested_mixture) {
        return;
    }

    if (previous < requested_mixture) {
        B = mixture_leaning_step_q8;
        const uint16_t fraction = (uint16_t)mixture_rate_fraction + B;
        mixture_rate_fraction = (uint8_t)fraction;
        ACC = mixture_rate_output + (fraction >> 8);
    } else {
        B = mixture_enrichment_step;
        mixture_rate_fraction = 0;
        ACC = mixture_rate_output - B;
        if (ACC < requested_mixture) {
            mixture_rate_output = requested_mixture;
            return;
        }
    }

    mixture_rate_output = ACC;
    requested_mixture = ACC;
}
