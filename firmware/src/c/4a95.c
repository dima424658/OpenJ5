#include "stock_registers.h"

enum {
    IGNITION_SLOTS_START = 0x80,
    IGNITION_SLOTS_END = 0x90,
    IGNITION_SLOT_BYTES = 4,
    COIL_OFFSET_MASK = 4,
    FIRST_COIL_REFERENCE = 47,
    SECOND_COIL_REFERENCE = 17,
    COIL_PHASE_WRAP_OFFSET = 39,
    TIMING_REFERENCE = 144,
    TIMING_BIAS = 132,
    WRAPPED_TIMING_ADJUSTMENT = 4,
    HALF_DEGREES_PER_SLOT = 12,
    SLOTS_PER_REVOLUTION = 60,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x5d) fuel_mode_flag_5d;
static __sbit __at (0xaf) interrupts_enabled;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x01) uint8_t bank_zero_r1;
static volatile __data __at (0x03) uint8_t bank_zero_r3;
static volatile __idata __at (0xa9) uint8_t cylinder_retard[4];
static volatile __xdata __at (0xf800) uint8_t dwell_slots;
static volatile __xdata __at (0xf801) uint8_t dwell_remainder_half_degrees;

extern __code const uint8_t ignition_subslot_ticks_raw[HALF_DEGREES_PER_SLOT];
extern void update_cooling_fan(void);

void write_ignition_timing_slots(void)
{
    stock_r0 = IGNITION_SLOTS_START;
    do {
        const uint8_t offset = stock_r0 - IGNITION_SLOTS_START;
        stock_r3 = (offset & COIL_OFFSET_MASK) ? SECOND_COIL_REFERENCE : FIRST_COIL_REFERENCE;
        stock_r1 = offset / IGNITION_SLOT_BYTES;
        B = cylinder_retard[stock_r1];

        ACC = TIMING_REFERENCE - stock_r2;
        if (fuel_mode_flag_5d) {
            ACC += B;
        }
        const uint8_t timing = ACC;
        ACC -= TIMING_BIAS;
        if (timing < TIMING_BIAS) {
            stock_r3 += COIL_PHASE_WRAP_OFFSET;
            ACC -= WRAPPED_TIMING_ADJUSTMENT;
        }

        const uint8_t fine = ACC % HALF_DEGREES_PER_SLOT;
        ACC = ACC / HALF_DEGREES_PER_SLOT + stock_r3;
        if (ACC >= SLOTS_PER_REVOLUTION) {
            ACC -= SLOTS_PER_REVOLUTION;
        }
        stock_r3 = ACC;
        stock_r4 = ACC;
        stock_r1 = fine;

        B = dwell_remainder_half_degrees;
        ACC = stock_r1 - B;
        if (stock_r1 < B) {
            ACC += HALF_DEGREES_PER_SLOT;
            --stock_r4;
        }
        B = ignition_subslot_ticks_raw[ACC];
        stock_r1 = ignition_subslot_ticks_raw[stock_r1];

        const uint8_t dwell = dwell_slots;
        const uint8_t ignition_slot = stock_r4;
        stock_r4 = dwell;
        ACC = ignition_slot - dwell;
        if (ignition_slot < dwell) {
            ACC += SLOTS_PER_REVOLUTION;
        } else if (ACC >= SLOTS_PER_REVOLUTION) {
            ACC -= SLOTS_PER_REVOLUTION;
        }

        volatile __idata uint8_t *entry = (volatile __idata uint8_t *)stock_r0;
        interrupts_enabled = 0;
        *entry++ = ACC;
        *entry++ = B;
        // The firing bytes use direct reads from the physical bank-zero addresses.
        *entry++ = bank_zero_r3;
        *entry++ = bank_zero_r1;
        interrupts_enabled = 1;
        stock_r0 = (uint8_t)entry;
    } while (stock_r0 != IGNITION_SLOTS_END);
    update_cooling_fan();
}
