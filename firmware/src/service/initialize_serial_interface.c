#include "stock_registers.h"

enum {
    SERVICE_SAVE_VALID = 0x55,
    SERVICE_TIMEOUT_LIMIT = 3,
    SERIAL_RX_DPTR = 6,
    SERIAL_TX_DPTR = 7,
};

static __sfr __at (0x87) PCON;
static __sfr __at (0x92) DPSEL;
static __sfr __at (0x98) SCON;
static __sfr __at (0xaa) serial_reload_low;
static __sfr __at (0xba) serial_reload_high;
static __sfr __at (0xd8) sfr_d8;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x29) service_mode_29;
static __sbit __at (0x68) startup_serial_mode;
static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) service_link_ready_6b;
static __sbit __at (0x78) serial_state_78;
static __sbit __at (0x7b) service_event_pending;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x99) serial_transmit_pending;
static __sbit __at (0x9c) serial_receive_enabled;
static __sbit __at (0xac) serial_interrupt_enabled;

static volatile __idata __at (0xa5) uint8_t serial_default_reload;
static volatile __idata __at (0x92) uint8_t serial_tx_state;
static volatile __idata __at (0x93) uint8_t serial_state_93;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __data __at (0x7e) uint8_t serial_rx_count;
static volatile __xdata __at (0xf4b6) uint8_t service_probe_count;
static volatile __xdata __at (0xf4b9) uint8_t service_timeout_count;
static volatile __xdata __at (0xf4ba) uint8_t service_probe_requested;
static volatile __xdata __at (0xf9a3) uint8_t service_save_marker;

#define SERIAL_RX_BUFFER 0xf680u
#define SERIAL_TX_BUFFER 0xf600u
#define SERIAL_MODE_SIGNATURE ((volatile const __code uint8_t *)0xb08c)

void reset_serial_receive_buffer(void);
void reset_serial_transmit_buffer(void);

void initialize_serial_interface(void)
{
    SCON = 0x40;
    PCON = 0x80;
    sfr_d8 = 0xc0;
    serial_default_reload = 0xd0;
    if (startup_serial_mode) {
        serial_reload_low = 0xd5;
        serial_reload_high = 0x01;
    } else {
        serial_reload_low = 0xd0;
        serial_reload_high = 0x03;
    }

    reset_serial_receive_buffer();
    reset_serial_transmit_buffer();
    serial_state_93 = 0;
    serial_state_78 = 1;
    service_inhibit_69 = 0;

    if (service_save_marker != SERVICE_SAVE_VALID &&
        service_timeout_count < SERVICE_TIMEOUT_LIMIT) {
        service_link_ready_6b = 1;
    }

    if (extra_state_enabled) {
        service_mode_29 = 1;
        B = SERIAL_MODE_SIGNATURE[0];
        if ((SERIAL_MODE_SIGNATURE[1] ^ B) != 0xff) {
            service_link_ready_6b = 0;
            startup_serial_mode = 0;
        }
    }

    service_probe_count = 0;
    service_probe_requested = 0;
    service_event_timer = 0;
    if (!startup_serial_mode) {
        service_mode_29 = 0;
        serial_receive_enabled = 1;
        serial_interrupt_enabled = 1;
    }
}

void reset_serial_receive_buffer(void)
{
    uint8_t saved_dpsel;

    serial_rx_count = 0;
    saved_dpsel = DPSEL;
    DPSEL = SERIAL_RX_DPTR;
    STOCK_DPTR = SERIAL_RX_BUFFER;
    DPSEL = saved_dpsel;
}

void reset_serial_transmit_buffer(void)
{
    uint8_t saved_dpsel;

    serial_tx_count = 0;
    serial_tx_state = 0;
    saved_dpsel = DPSEL;
    DPSEL = SERIAL_TX_DPTR;
    STOCK_DPTR = SERIAL_TX_BUFFER;
    DPSEL = saved_dpsel;
}

void start_serial_transmission(void)
{
    uint8_t saved_dpsel = DPSEL;

    DPSEL = SERIAL_TX_DPTR;
    STOCK_DPTR = SERIAL_TX_BUFFER;
    DPSEL = saved_dpsel;
    service_event_pending = 0;
    serial_state_78 = 0;
    service_dispatch_7c = 0;
    service_event_timer = 0;
    serial_transmit_pending = 1;
}
