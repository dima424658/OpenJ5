#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0x12) eeprom_io_failed;
static __sbit __at (0x70) immobilizer_signal;
static __sbit __at (0xaf) EA;
static volatile __data __at (0x79) uint8_t immobilizer_flags;
static volatile __xdata __at (0xf4bc) uint8_t received_nibble_count;
static volatile __xdata __at (0xf4c1) uint8_t received_record[3];
static volatile __xdata __at (0xf4c4) uint8_t received_record_status;
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;

extern void store_immobilizer_record_copies(void);

void process_immobilizer_message(void)
{
    ACC = immobilizer_status;
    if (ACC_BIT2) {
        goto full_record;
    }
    if (ACC_BIT4) {
        goto clear_receive;
    }
    ACC = received_nibble_count ^ 1;
    if (ACC == 0) {
        EA = 0;
        ACC = UINT8_MAX;
        goto store_record;
    }

clear_receive:
    immobilizer_status &= 0x7f;
    return;

full_record:
    if (ACC_BIT3) {
        return;
    }
    EA = 0;
    ACC = 0xfe;

store_record:
    received_record_status = ACC;
    immobilizer_status = ACC;
    STOCK_DPTR = (uint16_t)received_record;
    store_immobilizer_record_copies();
    EA = 1;
    if (eeprom_io_failed) {
        immobilizer_status &= 0x7f;
        return;
    }

    immobilizer_signal = 0;
    immobilizer_flags &= 0xfe;
    immobilizer_status |= 0x43;
}
