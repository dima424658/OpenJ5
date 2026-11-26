#include "eeprom_triplet.h"

enum {
    STATUS_MARKER_OFFSET = 0x9d,
    SERVICE_EVENT_MINIMUM = 10,
    SERVICE_LAST_ATTEMPT = 4,
    SERIAL_TX_DPTR = 7,
};

static __sfr __at (0x92) DPSEL;
static __sfr __at (0xbe) sfr_be;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x29) service_mode_29;
static __sbit __at (0x4d) shutdown_pending;
static __sbit __at (0x56) service_park_iac_56;
static __sbit __at (0x68) startup_serial_mode;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) service_link_ready_6b;
static __sbit __at (0x6d) vehicle_moving;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7e) service_serial_restart_done;
static __sbit __at (0x9c) serial_receive_enabled;

static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (0x2f) uint8_t serial_state_flags;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __xdata __at (0xf4b6) uint8_t service_probe_count;
static volatile __xdata __at (0xf4b9) uint8_t service_timeout_count;
static volatile __xdata __at (0xfa79) uint8_t saved_status_marker;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[6];

extern void initialize_serial_interface(void);
extern void start_serial_transmission(void);
extern void reset_serial_receive_buffer(void);
extern void end_service_probe(void);
extern void append_serial_checksum(void);

void build_service_probe_status(void);

void check_eeprom_record_9d(void)
{
    READ_TRIPLET(read_eeprom_bytes, write_eeprom_bytes, STATUS_MARKER_OFFSET);
    ACC = scratch[0];
    stock_r5 = ACC;
    saved_status_marker = ACC;
}

void write_eeprom_status_marker(void)
{
    stock_r0 = EEPROM_SCRATCH_ADDRESS;
    stock_r1 = TRIPLET_COPY_COUNT;
    do {
        *(volatile __idata uint8_t *)stock_r0 = ACC;
        ++stock_r0;
    } while (--stock_r1);
    EEPROM_IO(write_eeprom_bytes, STATUS_MARKER_OFFSET, TRIPLET_COPY_COUNT);
    wait_for_eeprom_write();
}

void handle_service_probe_event(void)
{
    if (service_event_timer < SERVICE_EVENT_MINIMUM) {
        return;
    }

    stock_r3 = service_probe_count;
    if (stock_r3 != SERVICE_LAST_ATTEMPT) {
        service_probe_count = stock_r3 + 1;
        start_serial_transmission();
        reset_serial_receive_buffer();
        return;
    }

    if (!service_serial_restart_done) {
        serial_receive_enabled = 0;
        sfr_be &= 0xf7;
        serial_state_flags = 0;
        startup_serial_mode = 1;
        initialize_serial_interface();
        service_serial_restart_done = 1;
        return;
    }

    if (extra_state_enabled && service_mode_29) {
        service_link_ready_6b = 0;
        if (!shutdown_pending) {
            ACC = service_timeout_count + 1;
            if (ACC != 0) {
                service_timeout_count = ACC;
            }
        }
    }
    end_service_probe();
}

void send_service_probe_status(void)
{
    ++service_probe_count;
    build_service_probe_status();
}

void build_service_probe_status(void)
{
    uint8_t saved_dpsel = DPSEL;

    DPSEL = SERIAL_TX_DPTR;
    STOCK_DPTR = (uint16_t)serial_tx_buffer;
    serial_tx_buffer[0] = 0x82;
    serial_tx_buffer[1] = 0xc0;
    serial_tx_buffer[2] = 0x10;
    serial_tx_buffer[3] = 0xa0;
    serial_tx_buffer[4] = (extra_state_enabled ? 1 : 0) |
                          (shutdown_pending ? 0 : 2) |
                          (service_park_iac_56 ? 4 : 0) |
                          (vehicle_moving ? 8 : 0);
    serial_tx_count = 5;
    DPSEL = saved_dpsel;
    append_serial_checksum();
    service_dispatch_7c = 1;
}
