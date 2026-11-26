static __sfr __at (0x92) DPSEL;
static __sfr __at (0xd0) PSW;
static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0x42) knock_sample_pending;

extern void prepare_knock_sample(void);
extern void continue_crank_event(void);

void crank_interrupt_entry(void) __naked
{
    // The return address is already on the stack. The shared exit at 0x202f pops this frame.
    __asm
        push _PSW
        push _ACC
        push _B
        push _DPSEL
    __endasm;
    DPSEL = 0;
    __asm
        push 0x82             ; DPL of bank 0
        push 0x83             ; DPH of bank 0
    __endasm;
    PSW = 0x08;

    if (knock_sample_pending) {
        prepare_knock_sample();
        return;
    }
    continue_crank_event();
}
