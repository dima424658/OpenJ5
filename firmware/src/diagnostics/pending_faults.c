#include "stock_registers.h"

enum { FORCE_ROM_FAULT_MASK = 0x80 };

static __sfr __at (0xf0) B;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x4c) ignition_supply_low;
static __sbit __at (0x4d) shutdown_pending;
static __sbit __at (0x69) service_inhibit_69;

static volatile __data __at (0x23) uint8_t diagnostic_low_signals;
static volatile __data __at (0x24) uint8_t diagnostic_high_signals;
static volatile __data __at (0x25) uint8_t diagnostic_io;
static volatile __data __at (0x5b) uint8_t reported_diagnostic_latches;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xf9b2) uint8_t driver_diagnostics_f9b2;
static volatile __xdata __at (0xf9b3) uint8_t driver_diagnostics_f9b3;
static volatile __xdata __at (0xf9b4) uint8_t driver_diagnostics_f9b4;
static volatile __xdata __at (0xf4ca) uint8_t pending_latch_faults;
static volatile __xdata __at (0xf4cb) uint8_t pending_low_signal_faults;
static volatile __xdata __at (0xf4cc) uint8_t pending_high_signal_faults;
static volatile __xdata __at (0xf4cd) uint8_t pending_io_faults;
static volatile __xdata __at (0xf4ce) uint8_t pending_faults_f4ce;
static volatile __xdata __at (0xf4cf) uint8_t pending_faults_f4cf;
static volatile __xdata __at (0xf4d0) uint8_t pending_faults_f4d0;
static volatile __xdata __at (0xf4d1) uint8_t pending_faults_f4d1;

extern volatile __code const uint8_t diagnostic_latch_mask;
extern volatile __code const uint8_t diagnostic_low_signal_mask;
extern volatile __code const uint8_t diagnostic_high_signal_mask;
extern volatile __code const uint8_t diagnostic_io_mask;
extern volatile __code const uint8_t retained_diagnostic_mask;
extern volatile __code const uint8_t driver_diagnostic_mask_1;
extern volatile __code const uint8_t driver_diagnostic_mask_2;
extern volatile __code const uint8_t driver_diagnostic_mask_3;

extern void continue_shutdown_outputs(void);
extern void update_lamp_and_outputs(void);
extern void read_driver_diagnostics(void);
extern void decode_driver_diagnostics(void);
extern void process_fault_history(void);

void accumulate_pending_faults(void);

void continue_after_fuel_learning(void)
{
    if (ignition_supply_low || shutdown_pending) {
        continue_shutdown_outputs();
        return;
    }
    if (service_inhibit_69) {
        update_lamp_and_outputs();
        return;
    }
    if (tick_1s) {
        read_driver_diagnostics();
        decode_driver_diagnostics();
    }
    accumulate_pending_faults();
}

void accumulate_pending_faults(void)
{
    B = (diagnostic_latch_mask | FORCE_ROM_FAULT_MASK) & reported_diagnostic_latches;
    pending_latch_faults |= B;
    B = diagnostic_low_signal_mask & diagnostic_low_signals;
    pending_low_signal_faults |= B;
    B = diagnostic_high_signal_mask & diagnostic_high_signals;
    pending_high_signal_faults |= B;
    B = diagnostic_io_mask & diagnostic_io;
    pending_io_faults |= B;

    B = retained_diagnostic_mask;
    B &= retained_status;
    pending_faults_f4ce |= B;
    B = driver_diagnostic_mask_1;
    B &= driver_diagnostics_f9b2;
    pending_faults_f4cf |= B;
    B = driver_diagnostic_mask_2;
    B &= driver_diagnostics_f9b3;
    pending_faults_f4d0 |= B;
    B = driver_diagnostic_mask_3;
    B &= driver_diagnostics_f9b4;
    pending_faults_f4d1 |= B;

    process_fault_history();
}
