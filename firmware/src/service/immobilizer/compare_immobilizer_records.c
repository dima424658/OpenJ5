#include "stock_registers.h"

static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;
static __sbit __at (0x6b) immobilizer_record_matched;
static __sbit __at (0x70) immobilizer_signal;
static __sbit __at (0xaf) EA;
static volatile __data __at (0x79) uint8_t immobilizer_flags;
static volatile __xdata __at (0xf4bd) uint8_t selected_record[3];
static volatile __xdata __at (0xf4c1) uint8_t received_record[3];
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;

extern void reset_after_supply_return(void);

void compare_immobilizer_records(void)
{
    ACC = immobilizer_status;
    if (ACC_BIT3) {
        return;
    }
    if (!ACC_BIT2) {
        immobilizer_status = ACC & 0x7f;
        return;
    }

    EA = 0;
    stock_r3 = 0;
    stock_r0 = selected_record[0];
    stock_r1 = selected_record[1];
    stock_r2 = selected_record[2];
    stock_r3 |= received_record[0] ^ stock_r0;
    stock_r3 |= received_record[1] ^ stock_r1;
    ACC = (received_record[2] ^ stock_r2) | stock_r3;
    EA = 1;
    if (ACC != 0) {
        immobilizer_status &= 0x7f;
        return;
    }

    immobilizer_signal = 0;
    immobilizer_flags &= 0xfe;
    immobilizer_record_matched = 1;
    immobilizer_status = (immobilizer_status & 0xdf) | 0x40;
    reset_after_supply_return();
}
