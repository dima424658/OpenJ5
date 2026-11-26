#include "stock_registers.h"

static __sfr __at (0x81) SP;
static __sfr __at (0x98) SCON;
static __sfr __at (0x99) SBUF;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xaf) EA;
static __sbit __at (0xe1) ACC_BIT1;

enum {
    MOV_A_DIRECT_OPCODE = 0xe5,
    MOV_DIRECT_A_OPCODE = 0xf5,
    RET_OPCODE = 0x22,
};

extern void append_monitor_reg_result(void);
extern void clear_flag_21_0_f275(void);

#define XDATA_BYTE (*(volatile __xdata uint8_t *)STOCK_DPTR)
#define STACK_TOP (*(volatile __idata uint8_t *)SP)
#define MONITOR_RX_CODE_ENTRY ((void (*)(void))UINT16_C(0xfa00))

void dispatch_monitor_register_frame(void)
{
    if (ACC != 0x31) {
        goto write_register;
    }
    if (stock_r3 != 1) {
        goto reject;
    }

    XDATA_BYTE = MOV_A_DIRECT_OPCODE;
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    ++STOCK_DPTR;
    goto finish_register_access;

write_register:
    if (ACC != 0x32) {
        goto transfer_control;
    }
    if (stock_r3 != 2) {
        goto reject;
    }

    XDATA_BYTE = MOV_DIRECT_A_OPCODE;
    ++STOCK_DPTR;
    stock_r0 = XDATA_BYTE;
    ++STOCK_DPTR;
    B = XDATA_BYTE;

finish_register_access:
    /* In monitor mode the receive buffer can also be fetched as CODE. */
    XDATA_BYTE = RET_OPCODE;
    ACC = B;
    MONITOR_RX_CODE_ENTRY();
    B = ACC;
    append_monitor_reg_result();
    return;

transfer_control:
    if (ACC != 0x41) {
        goto reject;
    }
    if (stock_r3 != 2) {
        goto reject;
    }

    EA = 0;
    SBUF = 0x0d;
    do {
        ACC = SCON;
    } while (!ACC_BIT1);
    SCON &= 0xfd;

    /* The monitor returns through the two bytes supplied in the frame. */
    ++STOCK_DPTR;
    ACC = XDATA_BYTE;
    ++SP;
    STACK_TOP = ACC;
    ++STOCK_DPTR;
    ACC = XDATA_BYTE;
    ++SP;
    STACK_TOP = ACC;
    SCON &= 0xfc;

reject:
    clear_flag_21_0_f275();
}
