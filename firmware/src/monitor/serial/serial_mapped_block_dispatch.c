#include "stock_registers.h"

static __sfr __at (0x82) DPL;
static __sfr __at (0x83) DPH;
static __sfr __at (0x92) DPSEL;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;

extern void append_monitor_reply_byte(void);
extern void read_monitor_mapped_byte(void);
extern void start_monitor_reply(void);
extern void append_monitor_result(void);
extern void reject_monitor_extended_frame(void);
extern void clear_flag_21_0_f275(void);
extern void dispatch_monitor_register_frame(void);

#define XDATA_BYTE (*(volatile __xdata uint8_t *)STOCK_DPTR)

void dispatch_monitor_block_frame(void)
{
    if (ACC == 0x23) {
        stock_r1 = 1;
        goto read_block;
    }
    if (ACC != 0x25) {
        goto write_external;
    }
    stock_r1 = 0;

read_block:
    if (stock_r3 != 3) {
        reject_monitor_extended_frame();
        return;
    }

    ++STOCK_DPTR;
    B = XDATA_BYTE;
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    ++STOCK_DPTR;
    stock_r3 = XDATA_BYTE;
    ACC = B;
    append_monitor_reply_byte();
    ACC = stock_r0;
    append_monitor_reply_byte();
    ACC = stock_r3;
    append_monitor_reply_byte();
    DPSEL = 1;
    DPH = stock_r0;
    DPL = B;
    do {
        read_monitor_mapped_byte();
        ++STOCK_DPTR;
        append_monitor_reply_byte();
    } while (--stock_r3);
    start_monitor_reply();
    return;

write_external:
    if (ACC == 0x24) {
        stock_r1 = 1;
        goto write_block;
    }
    if (ACC != 0x26) {
        dispatch_monitor_register_frame();
        return;
    }
    stock_r1 = 0;

write_block:
    ACC = stock_r3;
    if (ACC < 4) {
        reject_monitor_extended_frame();
        return;
    }
    B = ACC - 3;
    ++STOCK_DPTR;
    stock_r3 = XDATA_BYTE;
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    ++STOCK_DPTR;
    ACC = XDATA_BYTE;
    if (ACC != B) {
        clear_flag_21_0_f275();
        return;
    }

    ++STOCK_DPTR;
    const uint8_t low_address = stock_r3;
    stock_r3 = ACC;
    ACC = low_address;
    DPSEL = 1;
    DPH = stock_r0;
    DPL = ACC;
    DPSEL = 0;
    do {
        ACC = XDATA_BYTE;
        ++STOCK_DPTR;
        DPSEL = 1;
        XDATA_BYTE = ACC;
        B = ACC;
        read_monitor_mapped_byte();
        ++STOCK_DPTR;
        DPSEL = 0;
        if (ACC != B) {
            goto write_failed;
        }
    } while (--stock_r3);
    ACC = 0;
    goto reply;

write_failed:
    ACC = 1;

reply:
    append_monitor_result();
}
