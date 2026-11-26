#include "monitor_port9_config.h"
#include "stock_registers.h"

static __sfr __at (0x80) P0;
static __sfr __at (0x86) sfr_86;
static __sfr __at (0x87) PCON;
static __sfr __at (0x88) TCON;
static __sfr __at (0x89) TMOD;
static __sfr __at (0x90) P1;
static __sfr __at (0x92) DPSEL;
static __sfr __at (0x98) SCON;
static __sfr __at (0x9a) sfr_9a;
static __sfr __at (0xa0) P2;
static __sfr __at (0xa8) IE;
static __sfr __at (0xaa) sfr_aa;
static __sfr __at (0xb0) P3;
static __sfr __at (0xb1) sfr_b1;
static __sfr __at (0xb2) sfr_b2;
static __sfr __at (0xb4) sfr_b4;
static __sfr __at (0xb8) IP;
static __sfr __at (0xb9) sfr_b9;
static __sfr __at (0xba) sfr_ba;
static __sfr __at (0xbc) sfr_bc;
static __sfr __at (0xbe) sfr_be;
static __sfr __at (0xbf) sfr_bf;
static __sfr __at (0xc0) sfr_c0;
static __sfr __at (0xc1) sfr_c1;
static __sfr __at (0xc8) T2CON;
static __sfr __at (0xc9) sfr_c9;
static __sfr __at (0xd0) PSW;
static __sfr __at (0xd1) sfr_d1;
static __sfr __at (0xd8) sfr_d8;
static __sfr __at (0xe1) sfr_e1;
static __sfr __at (0xe8) P4;
static __sfr __at (0xf0) B;
static __sfr __at (0xf6) sfr_f6;
static __sfr __at (0xf8) P5;
static __sfr __at (0xf9) P9;
static __sfr __at (0xfa) P6;

static __sbit __at (0x08) monitor_response_pending;
static __sbit __at (0x09) monitor_transmitting;
static __sbit __at (0x0f) monitor_flag_0f;
static __sbit __at (0x94) port1_bit4;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xd7) CY;
static __sbit __at (0xde) sfr_d8_bit6;
static __sbit __at (0xdf) sfr_d8_bit7;

static volatile __data __at (0x20) uint8_t monitor_flags;
static volatile __data __at (0x3f) uint8_t monitor_request_kind;
static volatile __data __at (0x62) uint8_t monitor_field_62;

extern void resume_service_monitor(void);

void enter_alternate_startup(void)
{
    PSW = 0;
    stock_r0 = 0x29;
    stock_r1 = 7;
    ACC = 0;
    do {
        *((volatile __idata uint8_t *)stock_r0) = ACC;
        ++stock_r0;
    } while (--stock_r1);
    PCON = ACC;
    sfr_b1 = 0xa1;
    sfr_b2 = ACC;
    sfr_b4 = 0xd5;
    sfr_b1 |= 0x10;
    sfr_f6 = ACC;
    sfr_b1 &= 0xef;
    sfr_c1 = ACC;
    sfr_c9 = ACC;
    sfr_f6 = ACC;
    sfr_c0 = ACC;
    sfr_d1 = ACC;
    IE = ACC;
    IP = ACC;
    sfr_9a = ACC;
    TCON = ACC;
    TMOD = ACC;
    T2CON = ACC;
    sfr_e1 = 0x40;
    sfr_bc = 0x40;
    sfr_be = ACC;
    sfr_bf = ACC;
    sfr_d8 = ACC;
    DPSEL = ACC;

    sfr_b1 |= 0x40;
    sfr_b9 |= 0x80;
    P0 = 0xff;
    sfr_b9 |= 0x80;
    P1 = 0x24;
    sfr_b9 |= 0x80;
    P2 = 0xff;
    sfr_b9 |= 0x80;
    P3 = 0x35;
    sfr_b9 |= 0x80;
    P4 = 0;
    sfr_b9 |= 0x80;
    P5 = 0x90;
    sfr_b9 |= 0x80;
    P6 = 0x26;
    sfr_b9 |= 0x80;
    P9 = 0x40;
    sfr_b1 &= 0xfc;
    PCON |= 0x80;
    sfr_d8_bit7 = 1;
    sfr_86 = 0x80;
    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    ACC = P6;
    CY = !(ACC & 1);
    monitor_flag_0f = CY;
    monitor_request_kind = 0;
    monitor_flags = 0;
    monitor_field_62 = 0;
    sfr_ba = 0xff;
    sfr_aa = 0xcc;
    SCON = 0x50;
    monitor_response_pending = 0;
    monitor_transmitting = 0;
    port1_bit4 = 0;
    P9 |= 0x20;
    P9 &= 0xef;
    sfr_d8_bit6 = 1;
    sfr_b1 &= 0x7f;

    CONFIG_DELAY_32();
    ACC = 0x2c;
    SEND_PORT9_CONFIG();

    CONFIG_DELAY_32();
    ACC = 0x6c;
    SEND_PORT9_CONFIG();

    CONFIG_DELAY_32();
    ACC = 0x82;
    SEND_PORT9_CONFIG();

    CONFIG_DELAY_32();
    ACC = 0xdf;
    SEND_PORT9_CONFIG();

    CONFIG_DELAY_32();
    ACC = 0xe1;
    SEND_PORT9_CONFIG();

    CONFIG_DELAY_32();
    /* The layout places the monitor immediately after the final delay. */
    resume_service_monitor();
}
