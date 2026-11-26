#include "stock_registers.h"

static __sfr __at (0xbe) serial_control_be;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x7b) service_event_pending;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7f) serial_extended_length;
static __sbit __at (0x9c) serial_receive_enabled;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x73) uint8_t serial_expected_length;
static volatile __data __at (0x7e) uint8_t serial_rx_count;
static volatile __idata __at (0x91) uint8_t serial_rx_state;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __idata __at (0xa7) uint8_t serial_request_service;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern void reset_serial_receive_buffer(void);
extern void reset_serial_transmit_buffer(void);
extern void build_serial_probe_status(void);
extern void dispatch_valid_service_frame(void);

void respond_to_frame_error(void);
void restart_serial_receive(void);

void validate_service_frame(void)
{
    if (serial_rx_buffer[1] != 0x10) {
        ACC = 1;
    } else if ((serial_rx_buffer[0] & 0xc0) != 0x80) {
        ACC = 2;
    } else if (serial_rx_count > serial_expected_length) {
        ACC = 4;
    } else {
        /* A zero iteration count means 256 bytes in the stock byte loop. */
        stock_r4 = 0;
        stock_r3 = serial_expected_length - 1;
        STOCK_DPTR = (uint16_t)serial_rx_buffer;
        do {
            stock_r4 += *(volatile __xdata uint8_t *)STOCK_DPTR;
            ++STOCK_DPTR;
        } while (--stock_r3);
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR == stock_r4 ? 0 : 8;
    }
    serial_rx_state = ACC;
}

void route_service_frame(void)
{
    serial_response_address = serial_rx_buffer[2];
    serial_request_service = serial_extended_length ? serial_rx_buffer[4] : serial_rx_buffer[3];
    if (serial_rx_state) {
        respond_to_frame_error();
    } else {
        dispatch_valid_service_frame();
    }
}

void respond_to_frame_error(void)
{
    if (serial_rx_state & 3) {
        restart_serial_receive();
        return;
    }
    stock_r3 = (serial_rx_state & 4) ? 0x31 : 0x77;
    if (service_inhibit_69) {
        build_serial_probe_status();
    } else if (service_session_active) {
        reset_serial_transmit_buffer();
        STOCK_DPTR = (uint16_t)serial_tx_buffer;
        *(volatile __xdata uint8_t *)STOCK_DPTR = 0x83;
        stock_r4 = 0x83;
        const uint8_t fields[] = {
            serial_response_address, 0x10, 0x7f, serial_request_service, stock_r3,
        };
        for (uint8_t index = 0; index < sizeof(fields); ++index) {
            ++STOCK_DPTR;
            *(volatile __xdata uint8_t *)STOCK_DPTR = fields[index];
            if (index < sizeof(fields) - 1) {
                stock_r4 += fields[index];
            }
        }
        ACC = stock_r4 + stock_r3;
        ++STOCK_DPTR;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        serial_tx_count = 7;
        service_dispatch_7c = 1;
    } else {
        restart_serial_receive();
    }
}

void restart_serial_receive(void)
{
    serial_control_be &= 0xf7;
    service_event_pending = 0;
    reset_serial_receive_buffer();
    serial_rx_state = 0;
    serial_receive_enabled = 1;
}
