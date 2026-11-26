#include "stock_registers.h"

static __sfr __at (0x89) TMOD;
static __sfr __at (0x8a) TL0;
static __sfr __at (0x8c) TH0;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x8c) timer0_running;
static __sbit __at (0x8d) timer0_overflow;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xd7) CY;

enum {
    SOURCE_LOW,
    SOURCE_HIGH,
    EEPROM_OFFSET,
    BYTE_COUNT,
};

static volatile __xdata __at (0xfe82) uint8_t monitor_eeprom_original[4];
static volatile __xdata __at (0xfe86) uint8_t monitor_eeprom_progress[4];
static volatile __xdata __at (0xff00) uint8_t monitor_eeprom_readback[256];

extern void monitor_eeprom_write_page_step(void);
extern void monitor_eeprom_verify_fragment(void);
extern void monitor_eeprom_read_fragment(void);

void monitor_eeprom_write_and_verify(void)
{
    B = stock_r0;
    stock_r01 = STOCK_DPTR;
    monitor_eeprom_progress[SOURCE_LOW] = stock_r0;
    monitor_eeprom_progress[SOURCE_HIGH] = stock_r1;
    monitor_eeprom_original[SOURCE_LOW] = stock_r0;
    monitor_eeprom_original[SOURCE_HIGH] = stock_r1;
    monitor_eeprom_progress[EEPROM_OFFSET] = B;
    monitor_eeprom_original[EEPROM_OFFSET] = B;
    monitor_eeprom_progress[BYTE_COUNT] = stock_r2;
    monitor_eeprom_original[BYTE_COUNT] = stock_r2;

    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        monitor_eeprom_write_page_step();
        if (CY) {
            return;
        }

        TMOD = 1;
        timer0_running = 0;
        TL0 = 0xeb;
        TH0 = 0xcb;
        timer0_overflow = 0;
        timer0_running = 1;
        while (!timer0_overflow) {
        }
    } while (monitor_eeprom_progress[BYTE_COUNT] != 0);

    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    stock_r0 = monitor_eeprom_original[EEPROM_OFFSET];
    stock_r1 = monitor_eeprom_original[BYTE_COUNT];
    B = monitor_eeprom_original[SOURCE_HIGH];
    ACC = monitor_eeprom_original[SOURCE_LOW];
    STOCK_DPTR = ((uint16_t)B << 8) | ACC;
    monitor_eeprom_verify_fragment();
}

void monitor_eeprom_readback_256(void)
{
    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    STOCK_DPTR = (uint16_t)monitor_eeprom_readback;
    stock_r0 = 0;
    stock_r1 = 0;
    monitor_eeprom_read_fragment();
}
