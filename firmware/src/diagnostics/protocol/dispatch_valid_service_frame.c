#include "stock_registers.h"

static __sbit __at (0x69) service_inhibit_69;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __idata __at (0xa7) uint8_t serial_request_service;

extern void dispatch_diag_parameter(void);
extern void handle_service_18(void);
extern void handle_service_30(void);
extern void handle_service_14(void);
extern void handle_service_3e(void);
extern void handle_service_1a(void);
extern void handle_service_23(void);
extern void handle_service_3b(void);
extern void start_diagnostic_session(void);
extern void stop_diagnostic_session(void);
extern void stop_diagnostic_link(void);
extern void start_diagnostic_link(void);
extern void handle_diagnostic_service_11(void);
extern void handle_service_e0(void);
extern void handle_service_ea(void);
extern void handle_service_7f(void);
extern void handle_unrecognized_service(void);

void dispatch_valid_service_frame(void)
{
    stock_r4 = serial_request_service;
    switch (stock_r4) {
    case 0x21:
        dispatch_diag_parameter();
        return;
    case 0x18:
        handle_service_18();
        return;
    case 0x30:
        handle_service_30();
        return;
    case 0x14:
        handle_service_14();
        return;
    case 0x3e:
        handle_service_3e();
        return;
    case 0x1a:
        handle_service_1a();
        return;
    case 0x23:
        handle_service_23();
        return;
    case 0x3b:
        handle_service_3b();
        return;
    case 0x10:
        start_diagnostic_session();
        return;
    case 0x20:
        stop_diagnostic_session();
        return;
    case 0x82:
        stop_diagnostic_link();
        return;
    case 0x81:
        start_diagnostic_link();
        return;
    case 0x11:
        handle_diagnostic_service_11();
        return;
    }

    if (service_inhibit_69) {
        switch (stock_r4) {
        case 0xe0:
            handle_service_e0();
            return;
        case 0xea:
            handle_service_ea();
            return;
        case 0x7f:
            handle_service_7f();
            return;
        }
    }
    handle_unrecognized_service();
}
