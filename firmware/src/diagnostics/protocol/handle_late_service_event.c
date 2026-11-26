#include "negative_service_reply.h"

static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;

extern void start_serial_transmission(void);
extern void reset_serial_receive_buffer(void);

void handle_late_service_event(void)
{
    if (service_session_active) {
        WRITE_NEGATIVE_SERVICE_REPLY(0x21);
        service_dispatch_7c = 1;
        start_serial_transmission();
        reset_serial_receive_buffer();
    }
}
