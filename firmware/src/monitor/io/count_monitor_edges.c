#include "stock_registers.h"

static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x05) monitor_request_invalid;
static __sbit __at (0xb4) port3_bit4;
static __sbit __at (0xc5) monitor_edge_pending;
static __sbit __at (0xed) monitor_output_ed;

static volatile __data __at (0x3e) uint8_t monitor_output_request;
static volatile __data __at (0x3f) uint8_t monitor_request_kind;
static volatile __data __at (0x46) uint8_t monitor_pulse_count_low;
static volatile __data __at (0x47) uint8_t monitor_pulse_count_high;

extern void poll_monitor_exit_and_io(void);
extern void resume_service_monitor(void);

void count_monitor_edges(void)
{
    monitor_pulse_count_low = 0;
    monitor_pulse_count_high = 0;
    if (monitor_request_kind != 0x30) {
        monitor_command_active = 0;
        monitor_request_invalid = 1;
        resume_service_monitor();
        return;
    }

    port3_bit4 = monitor_output_request & 1;
    for (;;) {
        if (monitor_edge_pending) {
            monitor_edge_pending = 0;
            monitor_output_ed = !monitor_output_ed;
            ACC = monitor_pulse_count_low + 1;
            monitor_pulse_count_low = ACC;
            if (ACC == 0) {
                /* The stock path replaces the high byte on each rollover. */
                monitor_pulse_count_high = 0;
                monitor_pulse_count_high = 1;
            }
        }
        poll_monitor_exit_and_io();
    }
}
