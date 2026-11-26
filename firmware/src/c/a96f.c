#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr __at (0xf9) driver_bus_f9;
static __sfr __at (0xfa) driver_control_fa;
static __sbit __at (0xec) serial_clock;
static __sbit __at (0xfc) serial_data_in;
static __sbit __at (0x56) driver_diagnostics_enabled;

static volatile __data __at (0x4b) uint8_t driver_gate_input_4b;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xf9b8) uint8_t injector_driver_response;
static volatile __xdata __at (0xf9b9) uint8_t evap_driver_response;
static volatile __xdata __at (0xf9bb) uint8_t driver_gate_fault_count;

extern volatile __code const uint8_t driver_gate_level;
extern volatile __code const uint8_t driver_gate_fault_delay;

extern void update_relay_outputs(void);

void reverse_driver_response_bits(void);

#define NOP() __asm nop __endasm
#define SHIFT_DRIVER_BIT()                                                   \
    do {                                                                     \
        ACC = (uint8_t)((ACC >> 1) | (serial_data_in ? 0x80 : 0));           \
    } while (0)

#define READ_AFTER_FALLING_EDGE()                                            \
    do {                                                                     \
        do {                                                                 \
            serial_clock = 1;                                                \
            serial_clock = 0;                                                \
            SHIFT_DRIVER_BIT();                                              \
        } while (--stock_r2);                                                \
        reverse_driver_response_bits();                                      \
    } while (0)

#define READ_BEFORE_FALLING_EDGE()                                           \
    do {                                                                     \
        do {                                                                 \
            serial_clock = 1;                                                \
            NOP();                                                           \
            SHIFT_DRIVER_BIT();                                              \
            serial_clock = 0;                                                \
        } while (--stock_r2);                                                \
    } while (0)

void read_driver_diagnostics(void)
{
    if (serial_data_in) {
        driver_bus_f9 &= 0xfd;
        stock_r2 = 8;
        NOP();
        READ_AFTER_FALLING_EDGE();
        injector_driver_response = ACC;
        stock_r2 = 8;
        READ_AFTER_FALLING_EDGE();
        evap_driver_response = ACC;
    } else {
        driver_bus_f9 &= 0xfd;
        stock_r2 = 7;
        NOP();
        NOP();
        SHIFT_DRIVER_BIT();
        NOP();
        READ_BEFORE_FALLING_EDGE();
        injector_driver_response = ACC;

        stock_r2 = 8;
        READ_BEFORE_FALLING_EDGE();
        evap_driver_response = ACC;
    }

    driver_bus_f9 |= 2;
    if (driver_gate_input_4b > driver_gate_level && !(driver_control_fa & 0x20) &&
        driver_diagnostics_enabled) {
        B = driver_gate_fault_delay;
        ACC = driver_gate_fault_count + 1;
        driver_gate_fault_count = ACC;
        if (ACC >= B) {
            retained_status |= 0x10;
        }
    } else {
        driver_gate_fault_count = 0;
        retained_status &= 0xef;
    }

    update_relay_outputs();
}

void reverse_driver_response_bits(void)
{
    uint8_t input = ACC;
    uint8_t reversed = 0;

    for (uint8_t bit = 0; bit < 8; ++bit) {
        reversed = (uint8_t)((reversed << 1) | (input & 1));
        input >>= 1;
    }
    B = reversed;
    ACC = B;
}
