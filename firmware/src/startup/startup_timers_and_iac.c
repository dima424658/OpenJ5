#include "stock_registers.h"

enum { STARTUP_IAC_STATE_VALID = 0x55 };

static __sfr __at (0x82) DPL;
static __sfr __at (0x83) DPH;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0x8b) TL1;
static __sfr __at (0x8d) TH1;
static __sfr __at (0x92) DPSEL;
static __sfr __at (0xf0) B;
static __sbit __at (0x01) evap_fitted;
static __sbit __at (0x15) startup_mode_15;
static __sbit __at (0x16) retained_ram_checksum_failed;
static __sbit __at (0x48) timer_event_48;
static __sbit __at (0x5e) startup_purge_5e;
static __sbit __at (0x8e) timer1_running;
static __sbit __at (0xa8) irq_enable_0;
static __sbit __at (0xaa) irq_enable_2;
static __sbit __at (0xab) irq_enable_3;
static __sbit __at (0xae) iac_tick_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xe1) ACC_BIT1;

static volatile __data __at (0x46) uint8_t startup_state_46;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __xdata __at (0xf868) uint8_t startup_purge_duration;
static volatile __xdata __at (0xf8c2) uint8_t startup_iac_state_marker;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xfd00) uint8_t injection_correction_map[256];

extern volatile __code const uint8_t cold_purge_duration;
extern volatile __code const uint8_t hot_purge_duration;
extern volatile __code const uint8_t cold_purge_temperature;
extern __code const uint8_t purge_injection_correction[256];

extern void reset_saved_state_ac3c(void);
extern void reset_saved_state_ac95(void);
extern void home_and_seed_iac_feedback(void);
extern void restore_startup_iac_position(void);

void enable_startup_timers(void);
void select_startup_iac_position(void);

void continue_startup_sensor_mode(void)
{
    if (!evap_fitted) {
        goto start_timers;
    }

    startup_purge_5e = 1;
    B = cold_purge_temperature;
    if (coolant_temperature_raw < B) {
        goto cold_purge;
    }
    ACC = retained_status;
    if (startup_mode_15 || ACC_BIT1 || retained_ram_checksum_failed) {
        DPSEL = 0;
        STOCK_DPTR = (uint16_t)injection_correction_map;
        DPSEL = 2;
        STOCK_DPTR = (uint16_t)purge_injection_correction;
        do {
            B = *(const __code uint8_t *)STOCK_DPTR;
            ++STOCK_DPTR;
            DPSEL = 0;
            ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
            ACC += B;
            ++DPH;
            *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
            --DPH;
            ACC = ++DPL;
            DPSEL = 2;
        } while (ACC != 0);
        DPSEL = 0;
    }
    startup_purge_duration = hot_purge_duration;
    goto start_timers;

cold_purge:
    STOCK_DPTR = (uint16_t)injection_correction_map;
    do {
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
        ++DPH;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        --DPH;
    } while (--DPL != 0);
    startup_purge_duration = cold_purge_duration;

start_timers:
    enable_startup_timers();
}

void enable_startup_timers(void)
{
    startup_state_46 = 20;
    timer_event_48 = 0;
    TL1 = 0xcb;
    TH1 = 0xfa;
    timer1_running = 1;
    iac_tick_enabled = 1;
    iac_tick_high_priority = 1;
    irq_enable_0 = 1;
    irq_enable_3 = 1;
    irq_enable_2 = 0;
    select_startup_iac_position();
}

void select_startup_iac_position(void)
{
    if (startup_iac_state_marker == STARTUP_IAC_STATE_VALID) {
        goto restore_position;
    }
    if (retained_ram_checksum_failed) {
        reset_saved_state_ac3c();
        reset_saved_state_ac95();
    }
    ACC = retained_status;
    if (ACC_BIT1 || retained_ram_checksum_failed) {
        home_and_seed_iac_feedback();
        return;
    }

restore_position:
    restore_startup_iac_position();
}
