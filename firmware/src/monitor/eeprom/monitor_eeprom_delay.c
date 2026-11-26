#include "stock_registers.h"

static __sfr __at (0x90) P1;
static __sfr __at (0xb9) sfr_b9;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x95) eeprom_sda;
static __sbit __at (0xb3) eeprom_scl;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x62) uint8_t monitor_eeprom_device;

#define NOP() __asm nop __endasm

void monitor_eeprom_delay(void)
{
    NOP(); NOP(); NOP(); NOP(); NOP();
    NOP(); NOP(); NOP(); NOP(); NOP();
}

#define ROTATE_ACC_LEFT()                                 \
    do {                                                  \
        const uint8_t low_bit = CY;                       \
        CY = ACC_SIGN;                                    \
        ACC = (ACC << 1) | low_bit;                       \
    } while (0)

#define DRIVE_EEPROM_SDA()                                \
    do {                                                  \
        sfr_b9 |= 0x80;                                   \
        P1 &= 0xdf;                                       \
    } while (0)

#define RELEASE_EEPROM_SDA()                              \
    do {                                                  \
        sfr_b9 |= 0x80;                                   \
        P1 |= 0x20;                                       \
    } while (0)

#define MONITOR_EEPROM_STOP()                             \
    do {                                                  \
        DRIVE_EEPROM_SDA();                               \
        eeprom_sda = 0;                                   \
        monitor_eeprom_delay();                           \
        eeprom_scl = 1;                                   \
        monitor_eeprom_delay();                           \
        eeprom_sda = 1;                                   \
        RELEASE_EEPROM_SDA();                             \
        monitor_eeprom_delay();                           \
    } while (0)

void monitor_eeprom_send_byte(void)
{
    DRIVE_EEPROM_SDA();
    B = 8;
    do {
        ROTATE_ACC_LEFT();
        eeprom_sda = CY;
        monitor_eeprom_delay();
        eeprom_scl = 1;
        monitor_eeprom_delay();
        eeprom_scl = 0;
    } while (--B);

    eeprom_sda = 1;
    RELEASE_EEPROM_SDA();
    monitor_eeprom_delay();
    eeprom_scl = 1;
    monitor_eeprom_delay();
    CY = eeprom_sda;
    eeprom_scl = 0;
    monitor_eeprom_delay();
    if (CY) {
        MONITOR_EEPROM_STOP();
    }
}

void monitor_eeprom_read_byte(void)
{
    B = 8;
    eeprom_scl = 0;
    eeprom_sda = 1;
    RELEASE_EEPROM_SDA();
    do {
        monitor_eeprom_delay();
        eeprom_scl = 1;
        monitor_eeprom_delay();
        CY = eeprom_sda;
        ROTATE_ACC_LEFT();
        eeprom_scl = 0;
    } while (--B);

    DRIVE_EEPROM_SDA();
    eeprom_sda = 0;
    monitor_eeprom_delay();
    eeprom_scl = 1;
    monitor_eeprom_delay();
}

void monitor_eeprom_write_fragment(void)
{
    DRIVE_EEPROM_SDA();
    monitor_eeprom_delay();
    eeprom_sda = 0;
    monitor_eeprom_delay();
    eeprom_scl = 0;

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

    do {
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
        monitor_eeprom_send_byte();
        if (CY) {
            return;
        }
        ++STOCK_DPTR;
    } while (--stock_r1);

    MONITOR_EEPROM_STOP();
}
