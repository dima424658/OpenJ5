#include "stock_registers.h"

static __sbit __at (0x70) lamp_off_request;

static volatile __idata __at (0x93) uint8_t diagnostic_fault_count;
static volatile __data __at (0x22) uint8_t fault_mask_22;
static volatile __data __at (0x23) uint8_t fault_mask_23;
static volatile __data __at (0x24) uint8_t fault_mask_24;
static volatile __data __at (0x25) uint8_t fault_mask_25;
static volatile __data __at (0x5b) uint8_t displayed_fault_mask;
static volatile __data __at (0x7c) uint8_t output_flags;

static volatile __xdata __at (0xf9b0) uint8_t driver_fault_status;
static volatile __xdata __at (0xf9b2) uint8_t driver_fault_b2;
static volatile __xdata __at (0xf9b3) uint8_t driver_fault_b3;
static volatile __xdata __at (0xf9b4) uint8_t driver_fault_b4;
static volatile __xdata __at (0xfa6c) uint8_t fault_state_6c;
static volatile __xdata __at (0xfa6d) uint8_t fault_state_6d;
static volatile __xdata __at (0xf4ca) uint8_t pending_fault_masks[8];
static volatile __xdata __at (0xf4d2) uint8_t retained_faults[64];

extern void clear_retained_words_ce94(void);

void clear_diagnostic_faults(void)
{
    lamp_off_request = 1;
    ACC = 0;
    diagnostic_fault_count = ACC;
    fault_mask_22 = ACC;
    fault_mask_23 = ACC;
    fault_mask_24 = ACC;
    displayed_fault_mask = ACC;
    fault_mask_25 = ACC;
    output_flags = ACC;
    driver_fault_status = ACC;
    driver_fault_b2 = ACC;
    driver_fault_b3 = ACC;
    driver_fault_b4 = ACC;
    fault_state_6c = ACC;
    fault_state_6d = ACC;
    for (uint8_t i = 0; i < sizeof(pending_fault_masks); ++i) {
        pending_fault_masks[i] = ACC;
    }
    clear_retained_words_ce94();
    for (uint8_t i = 0; i < sizeof(retained_faults); ++i) {
        retained_faults[i] = ACC;
    }
}
