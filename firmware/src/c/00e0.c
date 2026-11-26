#include <stdbool.h>
#include <stdint.h>

static __sfr __at (0xc1) capture_compare_control;
static __sfr __at (0xc6) crank_capture_low;
static __sfr __at (0xc7) crank_capture_high;

static __sbit __at (0x2a) crank_sync_lost;
static __sbit __at (0x31) crank_timing_ready;
static __sbit __at (0x32) pending_half_turn_a;
static __sbit __at (0x33) pending_half_turn_b;
static __sbit __at (0x35) crank_event;
static __sbit __at (0x39) previous_tooth_short;
static __sbit __at (0x3a) current_tooth_short;
static __sbit __at (0x3b) crank_gap_compare_active;
static __sbit __at (0xc5) capture_compare_pending;

static volatile __data __at (0x05) uint8_t crank_sync_state;
/* Bank 3 is private to the crank interrupt. */
static volatile __data __at (0x18) uint16_t measured_tooth_period;
static volatile __data __at (0x1a) uint16_t last_crank_capture;
static volatile __data __at (0x1c) uint16_t previous_tooth_period;
static volatile __data __at (0x1e) uint8_t crank_tooth;
static volatile __data __at (0x1f) uint8_t half_turn_tooth;

extern void irq_crank_epilogue(void);
extern void irq_crank_reti(void);
extern void dispatch_crank_tooth_event(void);
extern void reset_crank_tooth_and_phase(void);
extern void advance_crank_tooth(void);
void mark_crank_sync_lost(void);
void reset_crank_capture_state(void);

#define READ_CRANK_CAPTURE() ((uint16_t)crank_capture_low | ((uint16_t)crank_capture_high << 8))

void crank_gap_compare(void) __naked
{
    __asm
        push psw
        push acc
        push b
        mov psw,#0x18
    __endasm;
    last_crank_capture = READ_CRANK_CAPTURE();
    if (crank_tooth == 57) {
        const uint16_t next = last_crank_capture + previous_tooth_period;
        crank_capture_low = (uint8_t)next;
        crank_capture_high = next >> 8;
        capture_compare_pending = 0;
        ++crank_tooth;
        half_turn_tooth = 28;
    } else {
        crank_gap_compare_active = 0;
        capture_compare_control = (capture_compare_control & 0x3f) | 0x40;
        ++crank_tooth;
        half_turn_tooth = 29;
    }
    dispatch_crank_tooth_event();
}

void jump_to_crank_sync_loss(void)
{
    mark_crank_sync_lost();
}

void report_early_crank_event(void)
{
    crank_event = 1;
    reset_crank_capture_state();
}

void jump_to_crank_reti(void)
{
    irq_crank_reti();
}

void crank_capture_interrupt(void) __naked
{
    if (crank_gap_compare_active) {
        crank_gap_compare();
        return;
    }
    __asm
        push psw
        push acc
        push b
        mov psw,#0x18
    __endasm;
    measured_tooth_period = READ_CRANK_CAPTURE() - last_crank_capture;
    if (!crank_timing_ready) {
        report_early_crank_event();
        return;
    }

    const uint16_t period = previous_tooth_period;
    const uint16_t measured = measured_tooth_period;
    const uint16_t excess_limit = period >= 256 ? period : period / 2;
    if (crank_sync_state == 4) {
        if (measured < period) {
            if (period - measured <= period / 2) {
                advance_crank_tooth();
            } else if (current_tooth_short || previous_tooth_short) {
                mark_crank_sync_lost();
            } else {
                current_tooth_short = 1;
                irq_crank_epilogue();
            }
        } else if (measured - period <= excess_limit) {
            advance_crank_tooth();
        } else {
            mark_crank_sync_lost();
        }
        return;
    }

    if (crank_sync_state < 2) {
        ++crank_sync_state;
        goto update_period;
    }
    if (period <= UINT16_MAX / 2) {
        const uint16_t double_period = 2 * period;
        const bool long_tooth = measured >= double_period;
        const uint16_t deviation = long_tooth ? measured - double_period : double_period - measured;
        const bool candidate = deviation >= period - period / 2 &&
                               (deviation < period || deviation - period <= excess_limit);
        if (candidate) {
            if (!long_tooth) {
                crank_sync_state = 3;
                goto update_period;
            }
            if (crank_sync_state == 3) {
                last_crank_capture = READ_CRANK_CAPTURE();
                ++crank_sync_state;
                crank_sync_lost = 0;
                current_tooth_short = 0;
                previous_tooth_short = 0;
                reset_crank_tooth_and_phase();
                return;
            }
        }
    }
    crank_sync_state = 2;

update_period:
    previous_tooth_period = measured_tooth_period;
    last_crank_capture = READ_CRANK_CAPTURE();
    crank_event = 1;
    irq_crank_epilogue();
}

void mark_crank_sync_lost(void)
{
    crank_sync_lost = 1;
    reset_crank_capture_state();
}

void reset_crank_capture_state(void)
{
    crank_sync_state = 0;
    pending_half_turn_a = 0;
    pending_half_turn_b = 0;
    irq_crank_epilogue();
}
