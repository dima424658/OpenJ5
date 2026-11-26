#include "stock_registers.h"

enum {
    EEPROM_MARKER_PENDING = 0x55,
    EEPROM_MARKER_COMPLETE = 0xaa,
    POWER_DOWN_COOKIE = 0xa5,
};

static __sfr __at (0x90) port_1;
static __sfr __at (0xb1) sfr_b1;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x6a) shutdown_extra_state_6a;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static volatile __data __at (0x07) uint8_t stock_r7;
static volatile __idata __at (0xa8) uint8_t power_down_cookie;
static volatile __xdata __at (0xf942) uint8_t diagnostic_status;
static volatile __xdata __at (0xf9ac) uint8_t shutdown_eeprom_marker;
static volatile __xdata __at (0xf9b0) uint8_t retained_status_f9b0;
static volatile __xdata __at (0xfa43) uint8_t shutdown_eeprom_block[8];
static volatile __xdata __at (0xfa79) uint8_t status_marker_fa79;

extern void write_eeprom_status_marker(void);
extern void write_eeprom_shutdown_block(void);
extern void write_eeprom_diagnostic_state(void);
extern void write_eeprom_shutdown_words(void);
extern void service_extra_state_b73f(void);
extern void write_eeprom_block_f4a6(void);
extern void write_eeprom_block_f4a0(void);
extern void verify_eeprom_block_f4a0(void);
extern void verify_eeprom_block_f4a6(void);
extern void write_eeprom_shutdown_record(void);

void persist_shutdown_data(void);
void wait_for_power_removal(void);

void continue_power_down(void)
{
    if (diagnostic_status & 0x0f) {
        if (status_marker_fa79 == EEPROM_MARKER_PENDING) {
            goto persist;
        }
    } else if ((retained_status_f9b0 & 0x02) == 0) {
        goto persist;
    }

    ACC = EEPROM_MARKER_PENDING;
    write_eeprom_status_marker();

persist:
    persist_shutdown_data();
}

void persist_shutdown_data(void)
{
    STOCK_DPTR = (uint16_t)shutdown_eeprom_block;
    write_eeprom_shutdown_block();
    write_eeprom_diagnostic_state();
    write_eeprom_shutdown_words();
    sfr_b1 = 0;

    if (shutdown_extra_state_6a) {
        service_extra_state_b73f();
        shutdown_eeprom_marker = EEPROM_MARKER_PENDING;
    }

    stock_r7 = 0xff;
    write_eeprom_block_f4a6();
    if (shutdown_extra_state_6a) {
        write_eeprom_block_f4a0();
        verify_eeprom_block_f4a0();
    }
    verify_eeprom_block_f4a6();
    write_eeprom_shutdown_record();
    shutdown_eeprom_marker = EEPROM_MARKER_COMPLETE;
    power_down_cookie = POWER_DOWN_COOKIE;
    port_1 |= 0x80;
    wait_for_power_removal();
}

void wait_for_power_removal(void)
{
    for (;;) {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
    }
}
