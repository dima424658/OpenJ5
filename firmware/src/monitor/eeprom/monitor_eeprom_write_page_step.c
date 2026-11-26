#include "stock_registers.h"

static __sfr __at (0x90) P1;
static __sfr __at (0xb9) sfr_b9;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x95) eeprom_sda;
static __sbit __at (0xb3) eeprom_scl;
static __sbit __at (0xd5) verify_eeprom_data;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x62) uint8_t monitor_eeprom_device;
static volatile __xdata __at (0xfe86) uint8_t monitor_buffer_low;
static volatile __xdata __at (0xfe87) uint8_t monitor_buffer_high;
static volatile __xdata __at (0xfe88) uint8_t monitor_eeprom_offset;
static volatile __xdata __at (0xfe89) uint8_t monitor_eeprom_remaining;

extern void monitor_eeprom_delay(void);
extern void monitor_eeprom_send_byte(void);
extern void monitor_eeprom_read_byte(void);
extern void monitor_eeprom_write_fragment(void);

void monitor_eeprom_read_or_verify(void);

void monitor_eeprom_write_page_step(void)
{
    stock_r0 = monitor_eeprom_offset;
    stock_r2 = monitor_eeprom_remaining;
    const uint8_t buffer_low = monitor_buffer_low;
    const uint8_t buffer_high = monitor_buffer_high;
    STOCK_DPTR = ((uint16_t)buffer_high << 8) | buffer_low;

    const uint8_t page_space = 16 - (stock_r0 & 15);
    stock_r1 = stock_r2 < page_space ? stock_r2 : page_space;
    stock_r3 = stock_r1;
    monitor_eeprom_write_fragment();
    if (CY) {
        return;
    }

    stock_r0 += stock_r3;
    stock_r2 -= stock_r3;
    const uint16_t next_buffer = STOCK_DPTR;
    monitor_buffer_low = (uint8_t)next_buffer;
    monitor_buffer_high = next_buffer >> 8;
    monitor_eeprom_offset = stock_r0;
    monitor_eeprom_remaining = stock_r2;
    CY = 0;
}

void monitor_eeprom_verify_fragment(void)
{
    verify_eeprom_data = 1;
    monitor_eeprom_read_or_verify();
}

void monitor_eeprom_read_fragment(void)
{
    verify_eeprom_data = 0;
    monitor_eeprom_read_or_verify();
}

#define START_MONITOR_EEPROM_READ()                  \
    do {                                             \
        sfr_b9 |= 0x80;                              \
        P1 &= 0xdf;                                  \
        monitor_eeprom_delay();                      \
        eeprom_sda = 0;                              \
        monitor_eeprom_delay();                      \
        eeprom_scl = 0;                              \
    } while (0)

void monitor_eeprom_read_or_verify(void)
{
    START_MONITOR_EEPROM_READ();
    ACC = ((monitor_eeprom_device & 0x6f) << 1) | 0xa0;
    monitor_eeprom_send_byte();
    if (CY) {
        return;
    }
    ACC = stock_r0;
    monitor_eeprom_send_byte();
    if (CY) {
        return;
    }

    monitor_eeprom_delay();
    eeprom_scl = 1;
    START_MONITOR_EEPROM_READ();
    ACC = ((monitor_eeprom_device & 0x6f) << 1) | 0xa1;
    monitor_eeprom_send_byte();
    if (CY) {
        return;
    }

    do {
        monitor_eeprom_read_byte();
        if (verify_eeprom_data) {
            B = ACC;
            ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
            if (ACC != B) {
                CY = 1;
                return;
            }
        } else {
            *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        }
        ++STOCK_DPTR;
    } while (--stock_r1);

    eeprom_sda = 1;
    sfr_b9 |= 0x80;
    P1 |= 0x20;
    monitor_eeprom_delay();
    CY = 0;
}
