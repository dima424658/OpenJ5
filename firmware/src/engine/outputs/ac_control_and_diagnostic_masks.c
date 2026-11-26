#include "stock_registers.h"

enum {
    CRANK_FAULT_SAMPLE_LIMIT = 10,
    CRANK_FAULT_MASK = 0x01,
    PHASE_SENSOR_FAULT_MASK = 0x10,
    FORCE_ROM_FAULT_MASK = 0x80,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x10) crank_signal_fault;
static __sbit __at (0x2a) crank_sync_fault_sample;
static __sbit __at (0x45) knock_rpm_cutoff_active;
static __sbit __at (0x4a) tick_1min;
static __sbit __at (0x53) startup_flag_53;

static volatile __data __at (0x22) uint8_t diagnostic_latches;
static volatile __data __at (0x23) uint8_t diagnostic_low_signals;
static volatile __data __at (0x24) uint8_t diagnostic_high_signals;
static volatile __data __at (0x25) uint8_t diagnostic_io;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x54) uint8_t rpm_axis_code;
static volatile __data __at (0x5b) uint8_t reported_diagnostic_latches;
static volatile __xdata __at (0xf98b) uint8_t crank_fault_samples;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;

extern volatile __code const uint8_t knock_rpm_cutoff_raw;
extern volatile __code const uint8_t startup_exit_rpm_low;
extern volatile __code const uint8_t phase_diagnostic_min_rpm;
extern volatile __code const uint8_t diagnostic_latch_mask;
extern volatile __code const uint8_t diagnostic_low_signal_mask;
extern volatile __code const uint8_t diagnostic_high_signal_mask;
extern volatile __code const uint8_t diagnostic_io_mask;
extern volatile __code const uint8_t retained_diagnostic_mask;

extern void calculate_reported_fuel_usage(void);

void publish_diagnostic_masks(void);

void continue_after_ac_control(void)
{
    ACC = knock_rpm_cutoff_raw;
    knock_rpm_cutoff_active = ACC < rpm_axis_code;

    ACC = startup_exit_rpm_low;
    if (ACC < engine_rpm_10) {
        ACC = crank_fault_samples + crank_sync_fault_sample;
        if (ACC < CRANK_FAULT_SAMPLE_LIMIT) {
            crank_fault_samples = ACC;
        } else {
            crank_signal_fault = 1;
        }
    }
    publish_diagnostic_masks();
}

void publish_diagnostic_masks(void)
{
    reported_diagnostic_latches = diagnostic_latches;
    if (phase_diagnostic_min_rpm >= engine_rpm_10) {
        reported_diagnostic_latches &= (uint8_t)~PHASE_SENSOR_FAULT_MASK;
    }
    if (startup_flag_53) {
        reported_diagnostic_latches &= (uint8_t)~PHASE_SENSOR_FAULT_MASK;
    }
    reported_diagnostic_latches &= diagnostic_latch_mask | FORCE_ROM_FAULT_MASK;
    diagnostic_low_signals &= diagnostic_low_signal_mask;
    diagnostic_high_signals &= diagnostic_high_signal_mask;
    diagnostic_io &= diagnostic_io_mask;

    B = retained_diagnostic_mask;
    retained_status &= B;
    if (tick_1min) {
        diagnostic_latches &= (uint8_t)~CRANK_FAULT_MASK;
        crank_fault_samples = 0;
    }
    calculate_reported_fuel_usage();
}
