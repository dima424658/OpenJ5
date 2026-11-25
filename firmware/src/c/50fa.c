#include <stdint.h>

enum {
    SERVICE_PROBE_TIMEOUT = 150,
    SERVICE_STATUS_EVENT = 0x0c,
    SERVICE_AUTHORIZATION_FLAGS = 0x03,
    SERVICE_STOP_DELAY_ACTIVE = 0x40,
    LAMP_OVERRIDE = 0x01,
};

static __sfr __at (0xe0) ACC;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x68) shutdown_serial_mode;
static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x6a) shutdown_extra_state_6a;
static __sbit __at (0x6b) service_link_ready_6b;
static __sbit __at (0x70) lamp_off_request;
static __sbit __at (0x7b) service_event_pending;
static __sbit __at (0x7c) service_dispatch_7c;

static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __xdata __at (0xf4b6) uint8_t service_probe_count;
static volatile __xdata __at (0xf4b8) uint8_t service_probe_timer;
static volatile __xdata __at (0xf4b9) uint8_t service_timeout_count;
static volatile __xdata __at (0xf4ba) uint8_t service_probe_requested;
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_state;
static volatile __xdata __at (0xf4c6) uint8_t service_stop_delay;

extern void end_service_probe(void);
extern void send_service_probe_status(void);
extern void stop_engine_outputs(void);
extern void step_immobilizer_state(void);
extern void validate_service_frame(void);
extern void route_service_frame(void);
extern void handle_service_probe_event(void);
extern void handle_diagnostic_service_event(void);
extern void check_service_save_request(void);

void continue_shutdown_outputs(void)
{
    if (shutdown_serial_mode) {
        goto check_save;
    }
    if (service_inhibit_69) {
        ACC = service_probe_timer + 1;
        service_probe_timer = ACC;
        if (ACC == SERVICE_PROBE_TIMEOUT) {
            if (shutdown_extra_state_6a) {
                ACC = service_timeout_count + 1;
                if (ACC != 0) {
                    service_timeout_count = ACC;
                }
                service_link_ready_6b = 0;
            }
            end_service_probe();
        } else if (service_probe_requested || service_probe_count == 0) {
            send_service_probe_status();
            service_probe_requested = 0;
            service_event_timer = SERVICE_STATUS_EVENT;
            service_event_pending = 1;
        }
    }

    ACC = immobilizer_state;
    if (!ACC_BIT1 || !service_link_ready_6b) {
        stop_engine_outputs();
        const uint8_t state = immobilizer_state;
        if ((state & SERVICE_AUTHORIZATION_FLAGS) == SERVICE_AUTHORIZATION_FLAGS) {
            goto route_event;
        }
        if (state & SERVICE_STOP_DELAY_ACTIVE) {
            if (!tick_1s) {
                goto route_event;
            }
            ACC = service_stop_delay - 1;
            service_stop_delay = ACC;
            if (ACC != 0) {
                goto route_event;
            }

            immobilizer_state = state & ~SERVICE_STOP_DELAY_ACTIVE;
            output_override_flags |= LAMP_OVERRIDE;
            lamp_off_request = 1;
        }
        step_immobilizer_state();
    }

route_event:
    if (service_event_pending) {
        if (!service_dispatch_7c) {
            validate_service_frame();
            route_service_frame();
        } else if (service_inhibit_69) {
            handle_service_probe_event();
        } else {
            handle_diagnostic_service_event();
            if (service_link_ready_6b) {
                service_timeout_count = 0;
            }
        }
    }

check_save:
    check_service_save_request();
}
