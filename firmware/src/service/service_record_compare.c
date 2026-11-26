#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xf0) B;
static __sbit __at (0x6b) service_link_ready;
static __sbit __at (0xbd) event_irq_high_priority;
static __sbit __at (0x4c) reset_inhibited;

enum {
    RECORD_POINTER_BANK = 3,
    REQUEST_POINTER_BANK = 4,
    FIRST_RECORD_SIZE = 6,
    SECOND_RECORD_SIZE = 8,
};

#define REQUEST_DATA_ADDRESS UINT16_C(0xf685)
#define FIRST_RECORD_ADDRESS UINT16_C(0xf4a0)
#define SECOND_RECORD_ADDRESS UINT16_C(0xf4a6)

extern void reset_after_supply_return(void);

#define CURRENT_BYTE (*(volatile __xdata uint8_t *)STOCK_DPTR)

void compare_service_record_f4a0(void)
{
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = REQUEST_POINTER_BANK;
    STOCK_DPTR = REQUEST_DATA_ADDRESS;
    DPSEL = RECORD_POINTER_BANK;
    STOCK_DPTR = FIRST_RECORD_ADDRESS;
    stock_r2 = FIRST_RECORD_SIZE;

    do {
        B = CURRENT_BYTE;
        ++STOCK_DPTR;
        DPSEL = REQUEST_POINTER_BANK;
        ACC = CURRENT_BYTE ^ B;
        if (ACC != 0) {
            goto mismatch;
        }
        ++STOCK_DPTR;
        DPSEL = RECORD_POINTER_BANK;
    } while (--stock_r2);

    service_link_ready = 1;
    if (!event_irq_high_priority && !reset_inhibited) {
        reset_after_supply_return();
    }
    goto restore_bank;

mismatch:
    service_link_ready = 0;

restore_bank:
    DPSEL = saved_dpsel;
}

void compare_service_record_f4a6(void)
{
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = REQUEST_POINTER_BANK;
    STOCK_DPTR = REQUEST_DATA_ADDRESS;
    DPSEL = RECORD_POINTER_BANK;
    STOCK_DPTR = SECOND_RECORD_ADDRESS;
    stock_r2 = SECOND_RECORD_SIZE;

    do {
        stock_r3 = CURRENT_BYTE;
        ++STOCK_DPTR;
        DPSEL = REQUEST_POINTER_BANK;
        ACC = CURRENT_BYTE ^ stock_r3;
        if (ACC != 0) {
            goto mismatch;
        }
        ++STOCK_DPTR;
        DPSEL = RECORD_POINTER_BANK;
    } while (--stock_r2);
    goto restore_bank;

mismatch:
    service_link_ready = 0;

restore_bank:
    DPSEL = saved_dpsel;
}
