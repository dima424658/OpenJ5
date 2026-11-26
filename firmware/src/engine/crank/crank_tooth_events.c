#include <stdint.h>

static __sfr __at (0x92) DPSEL;
static __sfr __at (0xdd) sfr_dd;

static __sbit __at (0x32) half_turn_capture_valid;
static __sbit __at (0x33) airflow_accumulator_valid;
static __sbit __at (0x34) defer_injection_slot;
static __sbit __at (0x35) crank_event;
static __sbit __at (0x3e) ignition_coil_order;
static __sbit __at (0x40) crank_airflow_pending;
static __sbit __at (0x42) knock_sample_pending;
static __sbit __at (0x6f) period_borrow;
static __sbit __at (0x89) crank_event_interrupt_pending;

static volatile __data __at (0x10) uint8_t ignition_pair_a[4];
static volatile __data __at (0x14) uint8_t ignition_pair_b[4];
static volatile __data __at (0x1a) uint16_t last_crank_capture;
static volatile __data __at (0x1c) uint16_t previous_tooth_period;
static volatile __data __at (0x1f) uint8_t half_turn_tooth;
static volatile __data __at (0x30) uint16_t running_maf_sum;
static volatile __data __at (0x32) uint8_t running_maf_count;
static volatile __data __at (0x33) uint8_t maf_sample_count;
static volatile __data __at (0x34) uint16_t maf_sample_sum;
static volatile __data __at (0x36) uint16_t last_half_turn_capture;
static volatile __data __at (0x38) uint16_t crank_period;
static volatile __data __at (0x3c) uint8_t injection_slot;
static volatile __data __at (0x3d) uint8_t injection_sector;
static volatile __data __at (0x3e) uint8_t next_injection_slot;
static volatile __data __at (0x3f) uint8_t next_injection_sector;
static volatile __data __at (0x40) uint8_t engine_sector;
static volatile __data __at (0x41) uint8_t injection_relative_sector;
static volatile __data __at (0x43) uint8_t knock_mask_index;

static volatile __idata __at (0xbe) uint8_t period_ring_cursor;

static volatile __xdata __at (0xf9c7) uint8_t crank_period_buffer[14];
static volatile __xdata __at (0xf9d5) uint8_t crank_period_index;

void publish_half_turn_period(void);
void prepare_tooth_knock_event(void);
extern void update_crank_angular_outputs(void);

void dispatch_crank_tooth_event(void)
{
    crank_event = 1;
    if (previous_tooth_period >= 0x0800) {
        half_turn_capture_valid = 0;
        airflow_accumulator_valid = 0;
    }

    if (defer_injection_slot) {
        defer_injection_slot = 0;
    } else if (half_turn_tooth == injection_slot) {
        if (airflow_accumulator_valid) {
            maf_sample_sum = running_maf_sum;
            maf_sample_count = running_maf_count;
            running_maf_sum = 0;
            running_maf_count = 0;
        }
        injection_relative_sector = (engine_sector - injection_sector) & 3;
        crank_airflow_pending = 1;
        crank_event_interrupt_pending = 1;
        injection_slot = next_injection_slot;
        injection_sector = next_injection_sector;
        defer_injection_slot = 1;
        airflow_accumulator_valid = 1;
    }
    publish_half_turn_period();
}

void publish_half_turn_period(void)
{
    if (half_turn_tooth == 0) {
        if (half_turn_capture_valid) {
            period_borrow = (sfr_dd & 1) != 0;
            crank_period = last_crank_capture - last_half_turn_capture - period_borrow;

            uint8_t cursor = period_ring_cursor + 2;
            if (cursor >= 0xbe) {
                cursor = 0xb0;
            }
            period_ring_cursor = cursor;
            volatile __idata uint8_t *period_slot = (volatile __idata uint8_t *)cursor;
            period_slot[0] = (uint8_t)crank_period;
            period_slot[1] = crank_period >> 8;

            const uint8_t saved_dpsel = DPSEL;
            DPSEL = 5;
            crank_period_index = cursor;
            const uint8_t offset = cursor - 0xb0;
            crank_period_buffer[offset] = (uint8_t)crank_period;
            crank_period_buffer[offset + 1] = crank_period >> 8;
            DPSEL = saved_dpsel;
        }
        last_half_turn_capture = last_crank_capture;
        half_turn_capture_valid = 1;
    }
    prepare_tooth_knock_event();
}

void prepare_tooth_knock_event(void)
{
    if (half_turn_tooth == 8) {
        knock_mask_index = engine_sector;
        knock_sample_pending = 1;
        crank_event_interrupt_pending = 1;

        const uint8_t index = knock_mask_index ^ 2;
        const uint8_t table_address = 0x80 + (uint8_t)((index << 2) | (index >> 6));
        const volatile __idata uint8_t *schedule = (const volatile __idata uint8_t *)table_address;
        volatile __data uint8_t *pair = ((knock_mask_index + ignition_coil_order) & 1)
                                           ? ignition_pair_b : ignition_pair_a;
        pair[0] = schedule[0];
        pair[1] = schedule[1];
        pair[2] = schedule[2];
        pair[3] = schedule[3];
    }
    update_crank_angular_outputs();
}
