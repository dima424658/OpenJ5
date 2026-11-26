#include "stock_registers.h"

static __sfr __at (0x82) DPL;
static __sfr __at (0x83) DPH;
static __sfr __at (0x92) DPSEL;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CARRY;

extern void append_monitor_reply_byte(void);
extern void start_monitor_reply(void);
extern void read_monitor_mapped_byte(void);
extern void reject_monitor_frame(void);
extern void dispatch_monitor_extended_frame(void);

#define XDATA_BYTE (*(volatile __xdata uint8_t *)STOCK_DPTR)
#define IRAM_BYTE (*(volatile __idata uint8_t *)stock_r0)

void dispatch_monitor_frame(void)
{
    if (ACC != 0x01) {
        goto read_internal;
    }
    if (stock_r3 != 0) {
        goto reject;
    }
    ACC = 9;
    goto reply_value;

read_internal:
    if (ACC != 0x11) {
        goto write_internal;
    }
    if (stock_r3 != 1) {
        goto reject;
    }
    CARRY = 0;
    goto access_internal;

write_internal:
    if (ACC != 0x12) {
        goto read_external;
    }
    if (stock_r3 != 2) {
        goto reject;
    }
    CARRY = 1;

access_internal:
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    if (CARRY) {
        ++STOCK_DPTR;
        ACC = XDATA_BYTE;
        IRAM_BYTE = ACC;
    }
    B = IRAM_BYTE;
    ACC = stock_r0;
    goto reply_address_and_value;

read_external:
    if (ACC == 0x13) {
        stock_r1 = 1;
        goto read_mapped;
    }
    if (ACC != 0x15) {
        goto write_external;
    }
    stock_r1 = 0;

read_mapped:
    if (stock_r3 != 2) {
        goto reject;
    }
    CARRY = 0;
    goto access_mapped;

write_external:
    if (ACC == 0x14) {
        stock_r1 = 1;
        goto write_mapped;
    }
    if (ACC != 0x16) {
        dispatch_monitor_extended_frame();
        return;
    }
    stock_r1 = 0;

write_mapped:
    if (stock_r3 != 3) {
        goto reject;
    }
    CARRY = 1;

access_mapped:
    ++STOCK_DPTR;
    B = XDATA_BYTE;
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    if (CARRY) {
        ++STOCK_DPTR;
        ACC = XDATA_BYTE;
    }
    DPSEL = 1;
    DPH = stock_r0;
    DPL = B;
    if (CARRY) {
        XDATA_BYTE = ACC;
    }
    read_monitor_mapped_byte();
    B = ACC;

    ACC = DPL;
    append_monitor_reply_byte();
    ACC = DPH;

reply_address_and_value:
    append_monitor_reply_byte();
    ACC = B;

reply_value:
    append_monitor_reply_byte();
    start_monitor_reply();
    return;

reject:
    reject_monitor_frame();
}
