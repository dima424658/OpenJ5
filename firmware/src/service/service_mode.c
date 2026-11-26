#include <stdint.h>

enum {
    INJECTOR_1 = 0x01,
    INJECTOR_2 = 0x02,
    INJECTOR_3 = 0x04,
    INJECTOR_4 = 0x08,
    INJECTOR_MASK = 0x0f,
};

static __sfr __at (0x90) P1;
static __sfr __at (0xdd) sfr_dd;
static __sfr __at (0xf8) sfr_f8;
static __sfr __at (0xfa) sfr_fa;

static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x74) injector_phase_1;
static __sbit __at (0x75) injector_phase_2;
static __sbit __at (0x76) injector_phase_3;
static __sbit __at (0x77) injector_phase_4;
static __sbit __at (0xe8) injector_output_1;
static __sbit __at (0xe9) injector_output_2;
static __sbit __at (0xea) injector_output_3;
static __sbit __at (0xeb) injector_output_4;

static volatile __data __at (0x7c) uint8_t output_flags_7c;
static volatile __xdata __at (0xf8b3) uint8_t port1_snapshot;
static volatile __xdata __at (0xf8b5) uint8_t sfr_dd_snapshot;
static volatile __xdata __at (0xf8b6) uint8_t sfr_f8_snapshot;
static volatile __xdata __at (0xf8b7) uint8_t sfr_fa_snapshot;
static volatile __xdata __at (0xf95d) uint8_t injector_test_delay;

extern void update_relay_outputs(void);
extern void continue_after_output_service(void);

#define SERVICE_INJECTOR(phase, output) do { \
    (output) = (phase); \
    if (!(phase)) { \
        const uint8_t delay = injector_test_delay; \
        if (delay != 0) { \
            injector_test_delay = delay - 1; \
        } else { \
            (phase) = 1; \
            output_flags_7c &= ~INJECTOR_MASK; \
        } \
    } \
} while (0)

void run_service_mode_53b1(void)
{
    port1_snapshot = P1;
    sfr_f8_snapshot = sfr_f8;
    sfr_fa_snapshot = sfr_fa;
    sfr_dd_snapshot = sfr_dd;

    if (!fuel_mode_flag_56) {
        if (output_flags_7c & INJECTOR_1) {
            SERVICE_INJECTOR(injector_phase_1, injector_output_1);
        } else if (output_flags_7c & INJECTOR_2) {
            SERVICE_INJECTOR(injector_phase_2, injector_output_2);
        } else if (output_flags_7c & INJECTOR_3) {
            SERVICE_INJECTOR(injector_phase_3, injector_output_3);
        } else if (output_flags_7c & INJECTOR_4) {
            SERVICE_INJECTOR(injector_phase_4, injector_output_4);
        }
    }

    if (!service_inhibit_69) {
        update_relay_outputs();
    }
    continue_after_output_service();
}
