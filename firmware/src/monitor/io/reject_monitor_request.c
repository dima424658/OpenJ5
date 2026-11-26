#include "stock_registers.h"

static __sfr __at (0xb2) sfr_b2;
static __sfr __at (0xfa) P6;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x05) monitor_request_invalid;
static __sbit __at (0x92) port1_bit2;
static __sbit __at (0xb5) port3_bit5;
static __sbit __at (0xed) monitor_output_ed;

static volatile __data __at (0x3f) uint8_t monitor_request_kind;

extern void poll_monitor_exit_and_io(void);
extern void resume_service_monitor(void);

void reject_monitor_request(void)
{
    monitor_command_active = 0;
    monitor_request_invalid = 1;
    resume_service_monitor();
}

#define MIRROR_INPUT(level)                          \
    for (;;) {                                       \
        monitor_output_ed = (level);                 \
        poll_monitor_exit_and_io();                  \
    }

void mirror_monitor_input(void)
{
    switch (monitor_request_kind) {
    case 0x08:
        MIRROR_INPUT(port1_bit2);
    case 0x09:
        MIRROR_INPUT(port3_bit5);
    case 0x0d:
        MIRROR_INPUT((P6 & 2) != 0);
    case 0x2f:
        MIRROR_INPUT((sfr_b2 & 2) == 0);
    default:
        reject_monitor_request();
    }
}
