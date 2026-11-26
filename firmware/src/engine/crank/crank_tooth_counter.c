#include <stdint.h>

static __sfr __at (0xc1) capture_compare_control;
static __sfr __at (0xc6) crank_capture_low;
static __sfr __at (0xc7) crank_capture_high;

static __sbit __at (0x05) cam_sensor_configured;
static __sbit __at (0x14) crank_cam_phase_fault;
static __sbit __at (0x30) previous_cam_level;
static __sbit __at (0x36) full_cycle_event;
static __sbit __at (0x39) previous_tooth_short;
static __sbit __at (0x3a) current_tooth_short;
static __sbit __at (0x3b) crank_gap_compare_active;
static __sbit __at (0x92) cam_level;
static __sbit __at (0x93) crank_output_latch;
static __sbit __at (0xc5) capture_compare_pending;

static volatile __data __at (0x18) uint16_t measured_tooth_period;
static volatile __data __at (0x1a) uint16_t last_crank_capture;
static volatile __data __at (0x1c) uint16_t previous_tooth_period;
static volatile __data __at (0x1e) uint8_t crank_tooth;
static volatile __data __at (0x1f) uint8_t half_turn_tooth;
static volatile __data __at (0x40) uint8_t engine_sector;

void reset_crank_tooth_and_phase(void);
extern void dispatch_crank_tooth_event(void);
extern void mark_crank_sync_lost(void);

void advance_crank_tooth(void)
{
    last_crank_capture = (uint16_t)crank_capture_low | ((uint16_t)crank_capture_high << 8);
    previous_tooth_short = current_tooth_short;
    current_tooth_short = 0;

    if (crank_tooth == 56) {
        previous_tooth_period = measured_tooth_period;
        crank_gap_compare_active = 1;
        capture_compare_control = (capture_compare_control & 0x3f) | 0x80;
        crank_output_latch = 1;
        const uint16_t next = last_crank_capture + previous_tooth_period;
        crank_capture_low = (uint8_t)next;
        crank_capture_high = next >> 8;
        capture_compare_pending = 0;
        ++crank_tooth;
        half_turn_tooth = 27;
    } else if (crank_tooth < 56) {
        previous_tooth_period = measured_tooth_period;
        ++crank_tooth;
        if (crank_tooth == 30) {
            half_turn_tooth = 0;
            engine_sector = (engine_sector + 1) & 3;
        } else {
            half_turn_tooth = crank_tooth < 30 ? crank_tooth : crank_tooth - 30;
        }
    } else {
        if (crank_tooth == 59) {
            reset_crank_tooth_and_phase();
        } else {
            mark_crank_sync_lost();
        }
        return;
    }
    dispatch_crank_tooth_event();
}

void reset_crank_tooth_and_phase(void)
{
    crank_tooth = 0;
    half_turn_tooth = 0;

    if (!cam_sensor_configured) {
        crank_cam_phase_fault = 0;
        engine_sector = (engine_sector + 1) & 2;
    } else if (cam_level == previous_cam_level) {
        crank_cam_phase_fault = 1;
        engine_sector = (engine_sector + 1) & 2;
    } else {
        crank_cam_phase_fault = 0;
        previous_cam_level = !previous_cam_level;
        engine_sector = previous_cam_level ? 2 : 0;
    }
    if (!engine_sector) {
        full_cycle_event = 1;
    }
    dispatch_crank_tooth_event();
}
