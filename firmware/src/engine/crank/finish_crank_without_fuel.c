#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr __at (0xf0) B;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0x44) mdu_interrupted;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x43) uint8_t cylinder_slot;
static volatile __xdata __at (0xf899) uint8_t crank_event_count;
static volatile __xdata __at (0xf93e) uint8_t crank_diagnostic_flags;
static volatile __xdata __at (0xfa5b) uint16_t crank_diagnostic_count;

extern volatile __code const uint8_t crank_diagnostic_limit[2];
extern void subtract_u16_register_pairs(void);

void count_crank_event(void);
void finish_crank_interrupt(void) __naked;

void finish_crank_without_fuel(void)
{
    if (cylinder_slot == 0) {
        ACC = crank_diagnostic_flags;
        B = ACC;
        stock_r01 = crank_diagnostic_count;
        if ((B & 1) && stock_r01 != UINT16_MAX) {
            ++stock_r01;
        }
        crank_diagnostic_count = stock_r01;

        stock_r2 = crank_diagnostic_limit[0];
        stock_r3 = crank_diagnostic_limit[1];
        subtract_u16_register_pairs();
        ACC = B;
        ACC_BIT1 = CY;
        crank_diagnostic_flags = ACC;
    }
    count_crank_event();
}

void count_crank_event(void)
{
    ++crank_event_count;
    finish_crank_interrupt();
}

void finish_crank_interrupt(void) __naked
{
    mdu_interrupted = 1;
    DPSEL = 0;

    // Restore the frame saved by the crank interrupt entry at 0x078c.
    __asm
        pop 0x83             ; DPH of bank 0
        pop 0x82             ; DPL of bank 0
        pop _DPSEL
        pop _B
        pop _ACC
        pop 0xd0             ; PSW
        reti
    __endasm;
}
