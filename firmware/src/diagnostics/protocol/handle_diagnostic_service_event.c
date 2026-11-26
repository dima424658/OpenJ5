#include <stdint.h>

static volatile __data __at (0x78) uint8_t service_event_timer;

extern void start_serial_transmission(void);
extern void reset_serial_receive_buffer(void);
extern void handle_late_service_event(void);

void handle_diagnostic_service_event(void)
{
    if (service_event_timer < 25) {
        return;
    }
    if (service_event_timer < 51) {
        start_serial_transmission();
        reset_serial_receive_buffer();
    } else {
        handle_late_service_event();
    }
}
