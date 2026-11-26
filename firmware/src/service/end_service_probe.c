#include "stock_registers.h"

static __sfr __at (0xbe) sfr_be;
static __sbit __at (0x29) service_mode_29;
static __sbit __at (0x69) service_probe_active;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7e) service_serial_restart_done;
static __sbit __at (0x9c) serial_receive_enabled;

static volatile __data __at (0x2f) uint8_t serial_state_flags;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __xdata __at (0xf4b7) uint8_t shutdown_mode;
static volatile __xdata __at (0xf4b8) uint8_t service_probe_timer;
static volatile __xdata __at (0xf4ba) uint8_t service_probe_requested;
static volatile __xdata __at (0xf4cd) uint8_t pending_io_faults;
static volatile __xdata __at (0xf4eb) uint8_t probe_state_f4eb;
static volatile __xdata __at (0xf52b) uint8_t probe_state_f52b;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

extern void capture_service_probe_options(void);
extern void restart_serial_receive(void);

void end_service_probe(void)
{
    serial_receive_enabled = 0;
    sfr_be &= 0xf7;
    service_probe_active = 0;
    service_serial_restart_done = 0;
    capture_service_probe_options();

    serial_state_flags = 1;
    if (!service_mode_29) {
        probe_state_f4eb = 0;
        probe_state_f52b = 0;
        pending_io_faults &= 0xfd;
    }
    service_event_timer = 0;
    service_probe_timer = 0;
    service_probe_requested = 0;
    ++shutdown_mode;
    restart_serial_receive();
}

void handle_service_7f(void)
{
    serial_tx_count = serial_tx_buffer[3] == 0xa0 ? 6 : 14;
    service_dispatch_7c = 1;
}
