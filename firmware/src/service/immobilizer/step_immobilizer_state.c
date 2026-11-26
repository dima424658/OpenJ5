#include "stock_registers.h"

static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;

static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe6) ACC_BIT6;
static __sbit __at (0xe7) ACC_BIT7;
static __sbit __at (0x49) immobilizer_input;
static __sbit __at (0x70) immobilizer_signal;

extern void poll_immobilizer_pulses(void);
extern void process_immobilizer_message(void);
extern void compare_immobilizer_records(void);

void step_immobilizer_state(void)
{
    ACC = immobilizer_status;
    if (ACC_BIT6) {
        return;
    }
    if (!ACC_BIT7) {
        if (immobilizer_input) {
            immobilizer_signal = !immobilizer_signal;
        }
        return;
    }

    poll_immobilizer_pulses();
    ACC = immobilizer_status;
    if (!ACC_BIT1) {
        process_immobilizer_message();
    } else if (!ACC_BIT0) {
        compare_immobilizer_records();
    }
}
