#include "stock_registers.h"

enum {
    EEPROM_PAGE0_WRITE = 0xa0,
    EEPROM_PAGE0_READ = 0xa1,
    EEPROM_PAGE1_WRITE = 0xa2,
    EEPROM_PAGE1_READ = 0xa3,
};

static __sfr __at (0x86) sfr_86;
static __sbit __at (0xae) iac_tick_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xd7) CARRY;
static volatile __data __at (4) uint8_t stock_r4;

extern void eeprom_bus_start(void);
extern void eeprom_bus_stop(void);
extern void eeprom_bus_send_byte(void);
extern void eeprom_bus_receive_byte(void);

/* R4 supplies the offset, R1 the length and R0 the indirect-RAM pointer.
 * Carry reports a rejected byte. The error exits leave out the stop pulse.
 */
#define EEPROM_BEGIN(control)                                    \
    do {                                                         \
        sfr_86 = 0x80;                                           \
        iac_tick_enabled = 1;                                    \
        iac_tick_high_priority = 1;                              \
        stock_r3 = stock_r4;                                     \
        eeprom_bus_start();                                      \
        stock_r4 = (control);                                    \
        eeprom_bus_send_byte();                                  \
        if (CARRY) {                                             \
            goto no_ack;                                         \
        }                                                        \
        stock_r4 = stock_r3;                                     \
        eeprom_bus_send_byte();                                  \
        if (CARRY) {                                             \
            goto no_ack;                                         \
        }                                                        \
    } while (0)

#define EEPROM_READ_FROM(control)                                \
    do {                                                         \
        eeprom_bus_start();                                      \
        stock_r4 = (control);                                    \
        eeprom_bus_send_byte();                                  \
        if (CARRY) {                                             \
            goto no_ack;                                         \
        }                                                        \
        do {                                                     \
            CARRY = stock_r1 == 1;                               \
            eeprom_bus_receive_byte();                           \
            *(volatile __idata uint8_t *)stock_r0 = stock_r4;    \
            ++stock_r0;                                          \
        } while (--stock_r1);                                    \
    } while (0)

#define EEPROM_WRITE_DATA()                                      \
    do {                                                         \
        do {                                                     \
            stock_r4 = *(volatile __idata uint8_t *)stock_r0;    \
            eeprom_bus_send_byte();                              \
            if (CARRY) {                                         \
                goto no_ack;                                     \
            }                                                    \
            ++stock_r0;                                          \
        } while (--stock_r1);                                    \
    } while (0)

void read_eeprom_bytes(void)
{
    EEPROM_BEGIN(EEPROM_PAGE0_WRITE);
    EEPROM_READ_FROM(EEPROM_PAGE0_READ);
    eeprom_bus_stop();
    CARRY = 0;
    return;

no_ack:
    CARRY = 1;
}

void write_eeprom_bytes(void)
{
    EEPROM_BEGIN(EEPROM_PAGE0_WRITE);
    EEPROM_WRITE_DATA();
    eeprom_bus_stop();
    CARRY = 0;
    return;

no_ack:
    CARRY = 1;
}

void read_eeprom_page1_bytes(void)
{
    EEPROM_BEGIN(EEPROM_PAGE1_WRITE);
    EEPROM_READ_FROM(EEPROM_PAGE1_READ);
    eeprom_bus_stop();
    CARRY = 0;
    return;

no_ack:
    CARRY = 1;
}

void write_eeprom_page1_bytes(void)
{
    EEPROM_BEGIN(EEPROM_PAGE1_WRITE);
    EEPROM_WRITE_DATA();
    eeprom_bus_stop();
    CARRY = 0;
    return;

no_ack:
    CARRY = 1;
}
