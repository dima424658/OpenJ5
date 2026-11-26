#include "stock_registers.h"

enum { WARM_RESTART_MASK = 0x01, OXYGEN_HEATER_ON_MASK = 0x02 };

static __sfr __at (0xf0) B;
static __sbit __at (0x00) oxygen_sensor_configured;
static __sbit __at (0x4e) oxygen_heater_active;
static __sbit __at (0x4f) oxygen_threshold_set_4f;
static __sbit __at (0x53) oxygen_monitor_reset_53;
static __sbit __at (0x58) crank_stopped;
static __sbit __at (0xfe) oxygen_heater_output_high;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __xdata __at (0xf80a) uint8_t startup_coolant_temperature_raw;
static volatile __xdata __at (0xf80d) uint8_t ignition_supply_raw;
static volatile __xdata __at (0xf810) uint8_t oxygen_diagnostic_status;
static volatile __xdata __at (0xf895) uint8_t exhaust_temperature_high;
static volatile __xdata __at (0xf8f2) uint8_t dew_point_air_mass_threshold;
static volatile __xdata __at (0xf8f7) uint8_t accumulated_air_mass_high;
static volatile __xdata __at (0xf9ad) uint8_t retained_flags;
static volatile __xdata __at (0xf9ae) uint8_t oxygen_heater_diagnostic_count;

extern volatile __code const uint8_t dew_point_coolant_low;
extern volatile __code const uint8_t dew_point_coolant_high;
extern volatile __code const uint8_t dew_point_air_mass_warm_restart;
extern volatile __code const uint8_t dew_point_air_mass_default;
extern volatile __code const uint8_t oxygen_heater_exhaust_cutoff;
extern volatile __code const uint8_t oxygen_heater_supply_cutoff;
extern volatile __code const uint8_t oxygen_heater_coolant_cutoff;

extern void check_oxygen_heater_resistance(void);
extern void service_oxygen_heater_early(void);
extern void update_idle_speed_target(void);

void dispatch_oxygen_heater(void);
void disable_oxygen_heater(void);

void check_exhaust_dew_point(void)
{
    if (!oxygen_monitor_reset_53 && !oxygen_threshold_set_4f) {
        B = startup_coolant_temperature_raw;
        ACC = dew_point_coolant_high;
        if (ACC < B) {
            oxygen_threshold_set_4f = 1;
        } else {
            ACC = dew_point_coolant_low;
            if (ACC < B && (retained_flags & WARM_RESTART_MASK)) {
                ACC = dew_point_air_mass_warm_restart;
            } else {
                ACC = dew_point_air_mass_default;
            }
            dew_point_air_mass_threshold = ACC;
            B = ACC;
            if (accumulated_air_mass_high >= B) {
                oxygen_threshold_set_4f = 1;
            }
        }
    }
    dispatch_oxygen_heater();
}

void dispatch_oxygen_heater(void)
{
    if (oxygen_sensor_configured && !crank_stopped) {
        B = oxygen_heater_supply_cutoff;
        if (ignition_supply_raw < B && oxygen_heater_coolant_cutoff < coolant_temperature_raw) {
            if (!oxygen_threshold_set_4f) {
                service_oxygen_heater_early();
                return;
            }
            B = oxygen_heater_exhaust_cutoff;
            if (exhaust_temperature_high < B) {
                check_oxygen_heater_resistance();
                return;
            }
        }
    }
    disable_oxygen_heater();
}

void disable_oxygen_heater(void)
{
    oxygen_heater_output_high = 1;
    oxygen_heater_active = 0;
    oxygen_diagnostic_status &= (uint8_t)~OXYGEN_HEATER_ON_MASK;
    oxygen_heater_diagnostic_count = 0;
    update_idle_speed_target();
}
