#include "stock_registers.h"

enum {
    KNOCK_ALTERNATE_ZONE = 0x20,
    HOLD_MIXTURE_OUTPUT = 0x01,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x03) knock_sensor_configured;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x26) maf_adc_high;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x55) fuel_mode_flag_55;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x5b) deceleration_fuel_cut;
static __sbit __at (0x5d) fuel_mode_flag_5d;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __data __at (0x61) uint8_t applied_mixture;
static volatile __data __at (0x7a) uint8_t control_7a;
static volatile __xdata __at (0xf89a) uint8_t poststart_mixture_seconds;
static volatile __xdata __at (0xf89b) uint16_t poststart_fuel_floor;
static volatile __xdata __at (0xf933) uint8_t knock_noise_zone;

extern volatile __code const uint8_t knock_min_coolant;
extern volatile __code const uint8_t poststart_mixture_enrichment;
extern __code const uint8_t mixture_limit_by_coolant[39];
extern __code const uint8_t poststart_fuel_decay_by_coolant[39];

extern void update_knock_history(void);
extern void update_knock_history_alternate(void);
extern void combine_knock_corrections(void);
extern void update_fuel_resume_mixture(void);
extern void select_fuel_adaptation(void);

void publish_fuel_mixture(void);
void apply_requested_mixture(void);

void continue_fuel_mixture_checks(void)
{
    if (knock_sensor_configured && fuel_mode_flag_5d &&
        !(maf_adc_low || maf_adc_high || throttle_adc_low || throttle_adc_high) &&
        knock_min_coolant < coolant_temperature_raw) {
        if (knock_noise_zone & KNOCK_ALTERNATE_ZONE) {
            update_knock_history_alternate();
        } else {
            update_knock_history();
        }
        combine_knock_corrections();
    }
    publish_fuel_mixture();
}

void publish_fuel_mixture(void)
{
    update_fuel_resume_mixture();
    deceleration_fuel_cut = 0;
    fuel_mode_flag_55 = 0;
    fuel_mode_flag_56 = 1;

    B = mixture_limit_by_coolant[coolant_nearest_index];
    if (B < requested_mixture) {
        requested_mixture = B;
    }

    ACC = poststart_mixture_seconds;
    if (ACC != 0) {
        if (tick_1s) {
            poststart_mixture_seconds = ACC - 1;
        }
        const uint8_t enrichment = poststart_mixture_enrichment;
        const uint8_t mixture = requested_mixture;
        requested_mixture = mixture >= enrichment ? mixture - enrichment : 0;
    }

    stock_r01 = poststart_fuel_floor;
    B = poststart_fuel_decay_by_coolant[coolant_nearest_index];
    const uint16_t floor = stock_r01;
    stock_r01 = floor >= B ? floor - B : 0;
    poststart_fuel_floor = stock_r01;

    apply_requested_mixture();
}

void apply_requested_mixture(void)
{
    if (!(control_7a & HOLD_MIXTURE_OUTPUT)) {
        applied_mixture = requested_mixture;
    }
    select_fuel_adaptation();
}
