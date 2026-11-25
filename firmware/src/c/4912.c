#include "stock_registers.h"

enum {
    HOLD_BASE_IGNITION = 0x04,
    APPLY_AUXILIARY_IGNITION = 0x08,
    DYNAMIC_RETARD_BIAS = 10,
    INJECTION_PHASE_SECTORS = 4,
    INJECTION_PHASE_SLOTS = 30,
    INJECTION_PHASE_PERIOD = INJECTION_PHASE_SECTORS * INJECTION_PHASE_SLOTS,
    INJECTION_PHASE_HALF_PERIOD = INJECTION_PHASE_PERIOD / 2,
    INJECTION_PHASE_REFERENCE = 11,
    INJECTION_PHASE_SECTOR_XOR = 2,
};

static __sfr __at (0xf0) B;
static __sbit __at (0xaf) interrupts_enabled;

static volatile __data __at (0x00) uint8_t bank_zero_r0;
static volatile __data __at (0x3c) uint8_t injection_slot;
static volatile __data __at (0x3d) uint8_t injection_sector;
static volatile __data __at (0x3e) uint8_t next_injection_slot;
static volatile __data __at (0x3f) uint8_t next_injection_sector;
static volatile __data __at (0x4d) uint8_t injection_phase;
static volatile __data __at (0x54) uint8_t rpm_axis_code;
static volatile __data __at (0x62) uint8_t base_ignition;
static volatile __data __at (0x7a) uint8_t ignition_control_7a;
static volatile __xdata __at (0xf88f) uint8_t auxiliary_ignition_f88f;
static volatile __xdata __at (0xf938) uint8_t dynamic_ignition_retard;
static volatile __xdata __at (0xf959) uint8_t target_injection_phase;
static volatile __xdata __at (0xf95a) uint8_t current_injection_phase;

extern volatile __code const uint8_t injection_phase_delay;
extern volatile __code const uint8_t injection_phase_min;
extern volatile __code const uint8_t injection_phase_max;
extern void clamp_stock_u8(void);
extern void restart_runtime_init(void);
extern void continue_ignition_output(void);

void update_injection_phase_step(void);

void finish_ignition_corrections(void)
{
    if (!(ignition_control_7a & HOLD_BASE_IGNITION) &&
        (ignition_control_7a & APPLY_AUXILIARY_IGNITION)) {
        base_ignition += auxiliary_ignition_f88f;
    }
    B = dynamic_ignition_retard;
    const uint8_t biased = base_ignition + DYNAMIC_RETARD_BIAS;
    const uint8_t limited = biased >= B ? biased - B : 0;
    base_ignition = limited - DYNAMIC_RETARD_BIAS;

    if (injection_sector >= INJECTION_PHASE_SECTORS || injection_slot >= INJECTION_PHASE_SLOTS) {
        restart_runtime_init();
        return;
    }
    update_injection_phase_step();
}

void update_injection_phase_step(void)
{
    if (injection_slot != next_injection_slot) {
        continue_ignition_output();
        return;
    }

    stock_r0 = injection_phase_min;
    stock_r1 = injection_phase_max;
    B = rpm_axis_code;
    ACC = injection_phase_delay;
    B = (uint16_t)ACC * B >> 8;

    const uint8_t biased = injection_phase + INJECTION_PHASE_HALF_PERIOD;
    ACC = biased - B;
    if (biased < B) {
        ACC += INJECTION_PHASE_PERIOD;
    }
    const uint8_t compensated = ACC;
    ACC -= INJECTION_PHASE_REFERENCE;
    if (compensated < INJECTION_PHASE_REFERENCE) {
        ACC += INJECTION_PHASE_PERIOD;
    }
    clamp_stock_u8();
    stock_r0 = ACC;
    target_injection_phase = ACC;

    ACC = (injection_sector ^ INJECTION_PHASE_SECTOR_XOR) * INJECTION_PHASE_SLOTS + injection_slot;
    stock_r1 = ACC;
    current_injection_phase = ACC;

    const uint8_t actual = ACC;
    // The comparison reads physical IRAM:00, independent of the active bank.
    const uint8_t target = bank_zero_r0;
    const uint8_t difference = actual - target;
    if (difference != 0) {
        if (actual > target) {
            if (difference >= INJECTION_PHASE_HALF_PERIOD) {
                ACC = stock_r1 + 1;
                if (ACC >= INJECTION_PHASE_PERIOD) {
                    ACC -= INJECTION_PHASE_PERIOD;
                }
            } else {
                ACC = stock_r1 - 1;
            }
        } else {
            if ((uint8_t)-difference >= INJECTION_PHASE_HALF_PERIOD) {
                ACC = stock_r1 - 1;
                if (stock_r1 == 0) {
                    ACC += INJECTION_PHASE_PERIOD;
                }
            } else {
                ACC = stock_r1 + 1;
            }
        }

        B = ACC % INJECTION_PHASE_SLOTS;
        ACC = (ACC / INJECTION_PHASE_SLOTS) ^ INJECTION_PHASE_SECTOR_XOR;
        interrupts_enabled = 0;
        next_injection_slot = B;
        next_injection_sector = ACC;
        interrupts_enabled = 1;
    }
    continue_ignition_output();
}
