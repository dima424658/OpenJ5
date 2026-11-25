#include "stock_registers.h"

enum { OXYGEN_HEATER_ADC_CHANNEL = 12, OXYGEN_HEATER_ON_MASK = 0x02 };

static __sfr __at (0xf0) B;
static __sbit __at (0x13) oxygen_heater_inhibited;
static __sbit __at (0x4e) oxygen_heater_active;
static __sbit __at (0xfe) oxygen_heater_output_high;

static volatile __xdata __at (0xf80e) uint8_t oxygen_heater_adc;
static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf9ae) uint8_t oxygen_heater_diagnostic_count;

extern volatile __code const uint8_t oxygen_heater_low_adc_limit;
extern volatile __code const uint8_t oxygen_heater_high_adc_limit;
extern volatile __code const uint8_t oxygen_heater_low_adc_ticks;

extern void read_adc_8bit(void);
extern void update_idle_speed_target(void);

void check_oxygen_heater_resistance(void)
{
    if (!oxygen_heater_inhibited) {
        oxygen_heater_output_high = 0;
    }
    oxygen_heater_active = 1;
    oxygen_diagnostic_status |= OXYGEN_HEATER_ON_MASK;
    B = OXYGEN_HEATER_ADC_CHANNEL;
    read_adc_8bit();
    oxygen_heater_adc = ACC;

    B = oxygen_heater_low_adc_limit;
    if (oxygen_heater_adc < B) {
        if (!oxygen_heater_inhibited) {
            B = oxygen_heater_low_adc_ticks;
            ACC = oxygen_heater_diagnostic_count + 1;
            oxygen_heater_diagnostic_count = ACC;
            if (ACC == B) {
                oxygen_heater_inhibited = 1;
                oxygen_heater_diagnostic_count = 0;
            }
        }
    } else {
        B = oxygen_heater_high_adc_limit;
        if (oxygen_heater_adc >= B) {
            if (!oxygen_heater_inhibited) {
                oxygen_heater_output_high = 1;
                oxygen_heater_inhibited = 1;
                oxygen_heater_active = 0;
                oxygen_diagnostic_status &= (uint8_t)~OXYGEN_HEATER_ON_MASK;
            }
        } else {
            oxygen_heater_diagnostic_count = 0;
            oxygen_heater_inhibited = 0;
        }
    }
    update_idle_speed_target();
}
