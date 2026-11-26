#include "stock_registers.h"

static __sfr __at (0x90) P1;
static __sfr __at (0xb9) sfr_b9;
static __sbit __at (0x95) eeprom_sda;
static __sbit __at (0xb3) eeprom_scl;
static __sbit __at (0xd7) CARRY;

static volatile __data __at (4) uint8_t stock_r4;

/* Keep the delays in instruction cycles so the line timing matches the ROM. */
#define NOP() __asm nop __endasm
#define WAIT_2() NOP(); NOP()
#define WAIT_3() WAIT_2(); NOP()
#define WAIT_4() WAIT_2(); WAIT_2()
#define WAIT_5() WAIT_4(); NOP()
#define WAIT_6() WAIT_4(); WAIT_2()
#define WAIT_8() WAIT_4(); WAIT_4()
#define WAIT_10() WAIT_8(); WAIT_2()
#define WAIT_12() WAIT_8(); WAIT_4()
#define WAIT_14() WAIT_12(); WAIT_2()

#define ROTATE_ACC_THROUGH_CARRY()                               \
    do {                                                         \
        uint8_t low_bit = CARRY;                                 \
        CARRY = ACC_SIGN;                                        \
        ACC = (uint8_t)((ACC << 1) | low_bit);                   \
    } while (0)

void eeprom_bus_start(void)
{
    eeprom_scl = 1;
    WAIT_14();
    eeprom_sda = 0;
    WAIT_14();
    eeprom_scl = 0;
    WAIT_12();
}

void eeprom_bus_stop(void)
{
    eeprom_sda = 0;
    WAIT_14();
    eeprom_scl = 1;
    WAIT_14();
    eeprom_sda = 1;
    WAIT_10();
}

void eeprom_bus_send_byte(void)
{
    ACC = stock_r4;
    stock_r2 = 8;

    do {
        ROTATE_ACC_THROUGH_CARRY();
        eeprom_sda = CARRY;
        WAIT_4();
        eeprom_scl = 1;
        WAIT_12();
        eeprom_scl = 0;
    } while (--stock_r2);

    WAIT_2();
    eeprom_sda = 1;
    WAIT_4();
    eeprom_scl = 1;
    WAIT_4();
    sfr_b9 |= 0x80;
    P1 |= 0x20;
    NOP();
    CARRY = eeprom_sda;
    WAIT_5();
    eeprom_scl = 0;
    WAIT_3();
    sfr_b9 |= 0x80;
    P1 &= 0xdf;
}

void eeprom_bus_receive_byte(void)
{
    stock_r2 = 8;
    /* Seed the shift register with the ACK level supplied by the caller.
     * After eight samples it rotates back into carry for the ninth clock.
     */
    ROTATE_ACC_THROUGH_CARRY();
    eeprom_sda = 1;
    WAIT_8();
    sfr_b9 |= 0x80;
    P1 |= 0x20;

    do {
        eeprom_scl = 1;
        WAIT_6();
        CARRY = eeprom_sda;
        WAIT_4();
        eeprom_scl = 0;
        ROTATE_ACC_THROUGH_CARRY();
        WAIT_6();
    } while (--stock_r2);

    sfr_b9 |= 0x80;
    P1 &= 0xdf;
    NOP();
    eeprom_sda = CARRY;
    WAIT_4();
    eeprom_scl = 1;
    WAIT_12();
    eeprom_scl = 0;
    stock_r4 = ACC;
    WAIT_3();
}
