#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) service_link_ready;
static __sbit __at (0x6c) shutdown_option;
static __sbit __at (0x7a) probe_saved;
static __sbit __at (0x7c) service_dispatch;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;

static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __xdata __at (0xf4b6) uint8_t service_probe_count;
static volatile __xdata __at (0xf4a6) uint8_t service_probe_record[8];
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[14];

extern void append_serial_checksum(void);
extern void copy_xdata_bytes(void);

static void build_service_probe_record(void);

#define SET_PROBE_HEADER()          \
    do {                            \
        serial_tx_buffer[0] = 0x8a; \
        serial_tx_buffer[1] = 0xc0; \
        serial_tx_buffer[2] = 0x10; \
        serial_tx_buffer[3] = 0xaa; \
    } while (0)

void send_service_probe_record(void)
{
    service_probe_count = 1;
    build_service_probe_record();
}

static void build_service_probe_record(void)
{
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = 7;
    SET_PROBE_HEADER();
    ACC = 0x80;
    ACC_BIT0 = service_link_ready;
    ACC_BIT1 = probe_saved;
    probe_saved = 0;
    ACC_BIT2 = shutdown_option;
    ACC_BIT3 = !extra_state_enabled;
    serial_tx_buffer[4] = ACC;

    DPSEL = 4;
    STOCK_DPTR = (uint16_t)&serial_tx_buffer[5];
    stock_r2 = sizeof(service_probe_record);
    DPSEL = 3;
    STOCK_DPTR = (uint16_t)service_probe_record;
    copy_xdata_bytes();

    serial_tx_count = 13;
    DPSEL = saved_dpsel;
    append_serial_checksum();
    service_dispatch = 1;
}

void build_serial_probe_status(void)
{
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = 7;
    SET_PROBE_HEADER();
    ACC = 0xc0;
    ACC_BIT0 = service_link_ready;
    ACC_BIT1 = probe_saved;
    ACC_BIT2 = shutdown_option;
    ACC_BIT3 = !extra_state_enabled;
    serial_tx_buffer[4] = ACC;
    serial_tx_buffer[5] = stock_r3;
    STOCK_DPTR = (uint16_t)&serial_tx_buffer[sizeof(serial_tx_buffer)];
    serial_tx_count = 13;
    DPSEL = saved_dpsel;
    append_serial_checksum();
    service_dispatch = 1;
}
