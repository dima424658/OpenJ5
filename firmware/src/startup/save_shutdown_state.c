#include "retained_ram.h"

enum { IGNITION_SUPPLY_ADC_CHANNEL = 1 };

static __sfr __at (0x86) sfr_86;
static __sfr __at (0xc1) sfr_c1;
static __sfr __at (0xc9) sfr_c9;
static __sfr __at (0xf0) B;
static __sfr __at (0xfa) sfr_fa;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0x4f) retained_flag_4f;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;

static volatile __data __at (0) uint8_t bank_zero_supply_threshold;
static volatile __data __at (0x47) uint8_t retained_byte_source;
static volatile __xdata __at (0xf8c2) uint8_t startup_iac_state_marker;
static volatile __xdata __at (0xf999) uint8_t relay_outputs;
static volatile __xdata __at (0xf9ad) uint8_t retained_flags;
static volatile __xdata __at (0xffcf) uint8_t retained_byte;

extern volatile __code const uint8_t shutdown_voltage_raw;

extern void filter_retained_entries(void);
extern void read_adc_8bit(void);
extern void restart_firmware(void);
extern void update_relay_outputs(void);
extern void continue_power_down(void);

void sum_retained_ram(void);
void recheck_shutdown_supply(void);
void prepare_power_down_outputs(void);

void save_shutdown_state(void)
{
    ACC = retained_flags;
    ACC_BIT0 = retained_flag_4f;
    retained_flags = ACC;
    retained_byte = retained_byte_source;
    sfr_86 = 0x80;
    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    filter_retained_entries();
    sum_retained_ram();
}

void sum_retained_ram(void)
{
    ACCUMULATE_RETAINED_RAM();
    retained_ram_checksum = stock_r01;
    recheck_shutdown_supply();
}

void recheck_shutdown_supply(void)
{
    bank_zero_supply_threshold = shutdown_voltage_raw;
    B = IGNITION_SUPPLY_ADC_CHANNEL;
    read_adc_8bit();
    if (ACC >= bank_zero_supply_threshold) {
        restart_firmware();
        return;
    }
    prepare_power_down_outputs();
}

void prepare_power_down_outputs(void)
{
    startup_iac_state_marker = 0;
    sfr_fa &= 0xef;
    sfr_c1 = 0;
    sfr_c9 = 0;
    relay_outputs &= 0xfb;
    update_relay_outputs();
    continue_power_down();
}
