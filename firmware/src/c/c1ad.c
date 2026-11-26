#include "eeprom_triplet.h"

static __sfr __at (0x86) sfr_86;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) service_link_ready;
static __sbit __at (0x6c) shutdown_option;
static __sbit __at (0x7a) probe_saved;
static __sbit __at (0xae) service_irq_enabled;
static __sbit __at (0xaf) EA;
static __sbit __at (0xbe) service_irq_high_priority;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;

static volatile __data __at (7) uint8_t stock_r7;
static volatile __xdata __at (0xf4a6) uint8_t probe_record[8];
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;
static volatile __xdata __at (0xf684) uint8_t request_options;

extern void compare_service_record_f4a0(void);
extern void compare_service_record_f4a6(void);
extern void write_startup_fault_history(void);
extern void copy_fault_history_from_scratch(void);
extern void capture_service_probe_options(void);
extern void write_eeprom_shutdown_record(void);
extern void build_serial_probe_status(void);
extern void send_service_probe_record(void);

void handle_service_e0(void)
{
    ACC = request_options;
    stock_r4 = ACC;
    if (extra_state_enabled) {
        goto enabled;
    }
    if (shutdown_option) {
        goto compare_long;
    }
    if (ACC_BIT2) {
        shutdown_option = 1;
        service_link_ready = 0;
        goto send_record;
    }
    if (ACC_BIT0 && ACC_BIT1) {
        goto clear_ready;
    }
    goto send_record;

compare_long:
    shutdown_option = 0;
    immobilizer_status &= 0xfd;
    compare_service_record_f4a6();
    if (!service_link_ready) {
        goto send_record;
    }
    ACC = stock_r4;
    if (!ACC_BIT2 || !ACC_BIT3) {
        goto send_record;
    }

    EA = 0;
    sfr_86 = 0x80;
    service_irq_enabled = 1;
    service_irq_high_priority = 1;
    probe_saved = 0;
    STOCK_DPTR = (uint16_t)probe_record;
    write_startup_fault_history();
    if (eeprom_io_failed) {
        goto write_failed;
    }

    copy_fault_history_from_scratch();
    probe_saved = 1;
    extra_state_enabled = 1;
    service_link_ready = 1;
    capture_service_probe_options();
    stock_r7 = 0;
    write_eeprom_shutdown_record();
    EA = 1;
    if (!eeprom_io_failed) {
        goto send_record;
    }

write_failed:
    EA = 1;
    stock_r3 = 0x10;
    build_serial_probe_status();
    return;

enabled:
    if (shutdown_option) {
        goto compare_long;
    }
    if (ACC_BIT2) {
        compare_service_record_f4a0();
        if (service_link_ready) {
            shutdown_option = 1;
        }
        goto send_record;
    }
    if (ACC_BIT1) {
        goto clear_ready;
    }
    compare_service_record_f4a0();
    goto send_record;

clear_ready:
    service_link_ready = 0;

send_record:
    send_service_probe_record();
}
