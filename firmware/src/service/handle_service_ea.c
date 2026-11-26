#include "stock_registers.h"

static __sfr __at (0xbe) sfr_be;
static __sbit __at (0x29) service_mode_29;
static __sbit __at (0x68) service_transition_pending;
static __sbit __at (0x69) service_probe_active;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) service_link_ready;
static __sbit __at (0x7e) service_serial_restart_done;
static __sbit __at (0x9c) serial_receive_enabled;

static volatile __data __at (0x2f) uint8_t serial_state_flags;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __xdata __at (0xf4b6) uint8_t service_probe_count;
static volatile __xdata __at (0xf4ba) uint8_t service_probe_requested;
static volatile __xdata __at (0xf684) uint8_t request_parameter;

extern void end_service_probe(void);

void handle_service_ea(void)
{
    ACC = request_parameter;
    if (!extra_state_enabled) {
        service_mode_29 = 0;
        goto end_probe;
    }

    ACC ^= 1;
    if (ACC != 0) {
        goto end_probe;
    }
    if (service_link_ready) {
        goto clear_service_mode;
    }
    ACC = service_probe_count ^ 2;
    if (ACC != 0) {
        goto clear_service_mode;
    }

    serial_receive_enabled = 0;
    sfr_be &= 0xf7;
    service_event_timer = 0x33;
    serial_tx_count = 3;
    stock_r0 = 0x91;
    *(volatile __idata uint8_t *)stock_r0 = 0;
    serial_state_flags = 0;
    service_probe_requested = 1;
    service_probe_active = 0;
    service_link_ready = 1;
    service_serial_restart_done = 1;
    service_transition_pending = 1;
    return;

clear_service_mode:
    service_mode_29 = 0;

end_probe:
    end_service_probe();
}
