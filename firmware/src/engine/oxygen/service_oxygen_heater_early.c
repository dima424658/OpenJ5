#include "stock_registers.h"

enum {
    OXYGEN_HEATER_ADC_CHANNEL = 12,
    OXYGEN_HEATER_PAUSE_POINTS = 16,
    EARLY_HEATER_ADC_OFFSET = 14,
    OXYGEN_READY_MASK = 0x01,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x13) oxygen_heater_inhibited;
static __sbit __at (0x2b) oxygen_heater_fault;
static __sbit __at (0x4e) oxygen_heater_active;
static __sbit __at (0xfe) oxygen_heater_output_high;

static volatile __xdata __at (0xf80e) uint8_t oxygen_heater_adc;
static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf8f1) uint8_t oxygen_heater_pause_limit;
static volatile __xdata __at (0xf8f3) uint8_t oxygen_heater_on_ticks;
static volatile __xdata __at (0xf8f4) uint8_t oxygen_heater_off_ticks;
static volatile __xdata __at (0xf978) uint8_t voltage_quantized;

extern volatile __code const uint8_t oxygen_heater_pulse_ticks;
extern __code const uint8_t oxygen_heater_pause_curve[OXYGEN_HEATER_PAUSE_POINTS];

extern void check_oxygen_heater_resistance(void);
extern void read_adc_8bit(void);
extern void update_idle_speed_target(void);

void service_oxygen_heater_early(void)
{
    if (!(oxygen_diagnostic_status & OXYGEN_READY_MASK) && !oxygen_heater_fault) {
        const uint8_t sample = oxygen_heater_adc;
        if (sample < EARLY_HEATER_ADC_OFFSET) {
            check_oxygen_heater_resistance();
            return;
        }
        ACC = sample - EARLY_HEATER_ADC_OFFSET;
    } else {
        ACC = oxygen_heater_pause_curve[voltage_quantized >> 4];
    }

    B = ACC;
    oxygen_heater_pause_limit = ACC;
    if (oxygen_heater_active) {
        ACC = oxygen_heater_on_ticks + 1;
        oxygen_heater_on_ticks = ACC;
        B = ACC;
        ACC = oxygen_heater_pulse_ticks;
        if (B >= ACC) {
            B = OXYGEN_HEATER_ADC_CHANNEL;
            read_adc_8bit();
            oxygen_heater_adc = ACC;
            oxygen_heater_output_high = 1;
            oxygen_heater_active = 0;
            oxygen_heater_off_ticks = 0;
        }
    } else {
        ACC = oxygen_heater_off_ticks + 1;
        if (ACC != 0) {
            oxygen_heater_off_ticks = ACC;
        }
        if (oxygen_heater_off_ticks >= B) {
            if (!oxygen_heater_inhibited) {
                oxygen_heater_output_high = 0;
            }
            oxygen_heater_active = 1;
            oxygen_heater_on_ticks = 0;
        }
    }
    update_idle_speed_target();
}
