#include "stock_registers.h"

static __sfr __at (0x89) TMOD;
static __sfr __at (0x8a) TL0;
static __sfr __at (0x8c) TH0;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;
static __sbit __at (0x8c) timer0_running;
static __sbit __at (0x8d) timer0_overflow;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x50) uint8_t monitor_selected_field;
static volatile __data __at (0x71) uint8_t monitor_serial_steps;

extern void step_monitor_serial_block(void);
extern void resume_service_monitor(void);

void request_monitor_serial_block(void)
{
    monitor_serial_steps = 11;
    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        step_monitor_serial_block();
        if (CY) {
            goto finish;
        }

        TMOD = 1;
        timer0_running = 0;
        TL0 = 0xeb;
        TH0 = 0xcb;
        timer0_overflow = 0;
        timer0_running = 1;
        while (!timer0_overflow) {
        }
    } while (monitor_serial_steps != 0);
    CY = 0;

finish:
    monitor_selected_field = monitor_serial_steps;
    monitor_checksum_failed = CY;
    monitor_command_active = 0;
    monitor_result_ready = 1;
    resume_service_monitor();
}
