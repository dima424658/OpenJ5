#include <stdint.h>

static __sfr __at (0x8b) timer1_low;
static __sfr __at (0x8d) timer1_high;
static __sfr __at (0x99) SBUF;
static __sfr __at (0xaa) serial_reload_low;
static __sfr __at (0xba) serial_reload_high;
static __sfr __at (0xbe) serial_control_be;
static __sfr __at (0xf8) P5;

static __sbit __at (0x48) tick_20ms;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x68) startup_serial_mode;
static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x79) service_request_79;
static __sbit __at (0x7a) service_flag_7a;
static __sbit __at (0x7b) service_event_pending;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_flag_7d;
static __sbit __at (0x7e) service_flag_7e;
static __sbit __at (0x8e) timer1_running;
static __sbit __at (0x9c) serial_receive_enabled;
static __sbit __at (0xac) serial_interrupt_enabled;
static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xf9) coil_1_level;
static __sbit __at (0xfa) coil_2_level;

static volatile __data __at (0x46) uint8_t tick_divider;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __data __at (0x7a) uint8_t ignition_override_7a;
static volatile __data __at (0x7b) uint8_t knock_override_7b;
static volatile __data __at (0x7c) uint8_t output_flags_7c;
static volatile __data __at (0x7d) uint8_t sensor_override_7d;

static volatile __idata __at (0x91) uint8_t serial_rx_state;
static volatile __idata __at (0x92) uint8_t serial_tx_state;
static volatile __idata __at (0xa5) uint8_t serial_default_reload;

static volatile __xdata __at (0xf603) uint8_t serial_reply_byte_3;
static volatile __xdata __at (0xf95e) uint8_t coil_test_duration;
static volatile __xdata __at (0xf95f) uint8_t coil_test_period;

void service_timer1_serial(void);
void service_timer1_coil_test(void);
extern void service_timer1_iac(void);
extern void reset_serial_receive_buffer(void);

void timer1_interrupt_entry(void) __naked
{
    __asm
        push psw
        push acc
        push b
        push dpl
        push dph
        mov psw,#0
        push 0x00
    __endasm;
    interrupts_enabled = 0;
    timer1_running = 0;
    if (timer1_high) {
        timer1_low = 255;
    }
    const uint16_t reload = 0xfacbu + timer1_low;
    timer1_low = (uint8_t)reload;
    timer1_high = reload >> 8;
    timer1_running = 1;
    interrupts_enabled = 1;

    if (!--tick_divider) {
        tick_divider = 20;
        tick_20ms = 1;
    }
    if (!startup_serial_mode) {
        service_timer1_serial();
        return;
    }

    ++service_event_timer;
    if (service_event_timer == 1) {
        SBUF = 0;
    } else if (service_event_timer == 50) {
        service_event_timer = 0;
        if (++serial_tx_count == 3) {
            service_event_timer = 50;
            serial_reload_low = 0xd0;
            serial_reload_high = 3;
        }
    }
    if (service_event_timer == 190) {
        serial_tx_count = 0;
        service_event_timer = 0;
        startup_serial_mode = 0;
        service_inhibit_69 = 1;
        serial_interrupt_enabled = 1;
    }
    service_timer1_iac();
}

void service_timer1_serial(void)
{
    if (service_inhibit_69) {
        ++service_event_timer;
        if (service_request_79) {
            if (!serial_receive_enabled) {
                if (service_event_timer >= 2) {
                    serial_rx_state = 0;
                    serial_receive_enabled = 1;
                }
            } else if (service_event_timer >= 200) {
                serial_receive_enabled = 0;
                service_request_79 = 0;
                service_event_pending = 1;
                service_dispatch_7c = 1;
                service_event_timer = 12;
                serial_tx_count = serial_reply_byte_3 == 0xa0 ? 6 : 14;
            }
        }
        service_timer1_iac();
        return;
    }

    if (!service_request_79) {
        if (serial_receive_enabled && !service_event_pending) {
            if (service_event_timer >= 40) {
                serial_receive_enabled = 0;
                serial_control_be &= 0xf7;
                service_flag_7a = 0;
                service_event_pending = 1;
                service_event_timer = 0;
            }
        } else if (service_event_pending) {
            ++service_event_timer;
        }
    }

    if (service_request_79 && ++service_event_timer == 100) {
        service_event_timer = 0;
        ++serial_tx_state;
        if (serial_tx_state == 1) {
            serial_reload_low = serial_default_reload;
            serial_rx_state = 0;
            serial_receive_enabled = 1;
        } else if (serial_tx_state >= 50) {
            serial_receive_enabled = 0;
            service_request_79 = 0;
            service_flag_7d = 0;
            service_flag_7e = 0;
            output_override_flags = 0;
            ignition_override_7a = 0;
            knock_override_7b = 0;
            output_flags_7c = 0;
            sensor_override_7d = 0;
            serial_rx_state = 0;
            reset_serial_receive_buffer();
            serial_receive_enabled = 1;
        }
    }
    service_timer1_coil_test();
}

void service_timer1_coil_test(void)
{
    if (!fuel_mode_flag_56 && (output_flags_7c & 0x30)) {
        const uint8_t remaining = coil_test_duration - 1;
        if (!remaining) {
            P5 |= 0x06;
        } else {
            coil_test_duration = remaining;
            if (!--coil_test_period) {
                coil_test_period = 5;
                if (output_flags_7c & 0x10) {
                    coil_1_level = !coil_1_level;
                } else {
                    coil_2_level = !coil_2_level;
                }
            }
        }
    }
    service_timer1_iac();
}
