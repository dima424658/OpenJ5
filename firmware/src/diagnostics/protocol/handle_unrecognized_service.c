#include "negative_service_reply.h"

static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x7c) service_dispatch_7c;

extern void build_serial_probe_status(void);

void handle_unrecognized_service(void)
{
    if (service_inhibit_69) {
        stock_r3 = 0x11;
        build_serial_probe_status();
    } else {
        WRITE_NEGATIVE_SERVICE_REPLY(0x11);
        service_dispatch_7c = 1;
    }
}
