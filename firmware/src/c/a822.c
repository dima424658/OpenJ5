#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x4b) sensor_batch_ready;

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __xdata __at (0xf833) uint8_t mixture_rate_output;
static volatile __xdata __at (0xf834) uint8_t fuel_mixture_ramp_ticks;
static volatile __xdata __at (0xf835) uint8_t fuel_mixture_ramp;

extern volatile __code const uint8_t fuel_resume_mixture_enrichment;
extern volatile __code const uint8_t fuel_mixture_ramp_period;
extern volatile __code const uint8_t warmed_engine_coolant_index;
extern __code const uint8_t base_mixture_by_coolant[39];
extern __code const uint8_t base_mixture_temp_correction[39];
extern __code const uint8_t catalyst_mixture_raw[256];

void update_fuel_resume_mixture(void)
{
    if (sensor_batch_ready) {
        const uint8_t ceiling = fuel_mixture_ramp;
        if (ceiling < requested_mixture) {
            requested_mixture = ceiling;
        }
    }

    B = fuel_mixture_ramp_period;
    ACC = fuel_mixture_ramp_ticks + 1;
    fuel_mixture_ramp_ticks = ACC;
    if (ACC == B) {
        fuel_mixture_ramp_ticks = 0;
        ACC = fuel_mixture_ramp + 1;
        if (ACC != 0) {
            fuel_mixture_ramp = ACC;
        }
    }
}

void update_fuel_cut_mixture(void)
{
    B = mixture_rate_output;
    if (fuel_mixture_ramp >= B) {
        fuel_mixture_ramp = B;
    }

    B = fuel_mixture_ramp_period;
    ACC = fuel_mixture_ramp_ticks + 1;
    fuel_mixture_ramp_ticks = ACC;
    if (ACC != B) {
        return;
    }
    fuel_mixture_ramp_ticks = 0;

    B = base_mixture_by_coolant[warmed_engine_coolant_index];
    const uint8_t catalyst_mixture = catalyst_mixture_raw[operating_point_index];
    B = catalyst_mixture >= B ? catalyst_mixture - B : 0;
    ACC = base_mixture_temp_correction[coolant_nearest_index];
    B = ((uint16_t)ACC * B) >> 8;

    // The stock target and candidate use arithmetic modulo 256.
    B = base_mixture_by_coolant[coolant_nearest_index] + B;
    B -= fuel_resume_mixture_enrichment;
    ACC = fuel_mixture_ramp - 1;
    if (ACC >= B) {
        fuel_mixture_ramp = ACC;
    }
}
