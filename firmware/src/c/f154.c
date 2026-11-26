#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;

extern void append_monitor_reply_byte(void);
extern void start_monitor_reply(void);
extern void append_monitor_result(void);
extern void clear_flag_21_0_f275(void);
extern void dispatch_monitor_block_frame(void);

#define XDATA_BYTE (*(volatile __xdata uint8_t *)STOCK_DPTR)
#define IRAM_BYTE (*(volatile __idata uint8_t *)stock_r0)

void dispatch_monitor_extended_frame(void)
{
    if (ACC != 0x21) {
        goto write_internal;
    }
    if (stock_r3 != 2) {
        goto reject;
    }

    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    ++STOCK_DPTR;
    stock_r3 = XDATA_BYTE;
    ACC = stock_r0;
    append_monitor_reply_byte();
    ACC = stock_r3;
    append_monitor_reply_byte();
    do {
        ACC = IRAM_BYTE;
        ++stock_r0;
        append_monitor_reply_byte();
    } while (--stock_r3);
    start_monitor_reply();
    return;

reject:
    clear_flag_21_0_f275();
    return;

write_internal:
    if (ACC != 0x22) {
        dispatch_monitor_block_frame();
        return;
    }
    ACC = stock_r3;
    if (ACC < 3) {
        goto reject;
    }
    B = ACC - 2;
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    ++STOCK_DPTR;
    ACC = XDATA_BYTE;
    if (ACC != B) {
        goto reject;
    }

    ++STOCK_DPTR;
    stock_r3 = ACC;
    do {
        ACC = XDATA_BYTE;
        ++STOCK_DPTR;
        IRAM_BYTE = ACC;
        B = ACC;
        ACC = IRAM_BYTE;
        ++stock_r0;
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
