#include <stdint.h>

static __sfr __at (0xa5) coil_compare_enable;
static __sfr __at (0xa6) coil_compare_flags;

static __sbit __at (0x11) pending_timer1_overflow;
static __sbit __at (0x3c) speed_input_was_low;
static __sbit __at (0x3d) second_ignition_coil;
static __sbit __at (0x51) early_cranking;
static __sbit __at (0x52) primary_spark_pending;
static __sbit __at (0x8f) timer1_overflow;
static __sbit __at (0xb5) vehicle_speed_input;
static __sbit __at (0xf9) coil_1_level;
static __sbit __at (0xfa) coil_2_level;

static volatile __data __at (0x44) uint8_t spark_phase_ticks;
static volatile __data __at (0x45) uint8_t remaining_sparks;
static volatile __data __at (0x7f) uint8_t speed_pulse_count;
static volatile __xdata __at (0xf978) uint8_t supply_voltage_raw;

extern __code const uint8_t additional_spark_interval_raw;
extern __code const uint8_t additional_spark_count;
extern __code const uint8_t ignition_dwell_raw[16];

void service_multispark_and_speed(void) __naked
{
    if (primary_spark_pending) {
        if (!early_cranking) {
            goto clear_primary_spark;
        }
        if (!second_ignition_coil) {
            if (!coil_1_level) {
                goto sample_speed;
            }
            coil_compare_enable &= 0xfd;
            coil_compare_flags &= 0xfd;
        } else {
            if (!coil_2_level) {
                goto sample_speed;
            }
            coil_compare_enable &= 0xfb;
            coil_compare_flags &= 0xfb;
        }
        remaining_sparks = additional_spark_count;
        if (!remaining_sparks) {
            goto clear_primary_spark;
        }
        goto reload_between_sparks;
    }
    if (!remaining_sparks || --spark_phase_ticks) {
        goto sample_speed;
    }

    /* A low output charges the selected coil. The rising edge fires it. */
    if (!second_ignition_coil) {
        if (!coil_1_level) {
            coil_1_level = 1;
            goto finish_spark;
        }
        coil_1_level = 0;
    } else {
        if (!coil_2_level) {
            coil_2_level = 1;
            goto finish_spark;
        }
        coil_2_level = 0;
    }

    spark_phase_ticks = (ignition_dwell_raw[supply_voltage_raw >> 4] >> 4) + 1;
    goto sample_speed;

finish_spark:
    if (!--remaining_sparks) {
        goto sample_speed;
    }

reload_between_sparks:
    spark_phase_ticks = additional_spark_interval_raw >> 4;

clear_primary_spark:
    primary_spark_pending = 0;

sample_speed:
    if (!vehicle_speed_input) {
        speed_input_was_low = 1;
    } else if (speed_input_was_low) {
        speed_input_was_low = 0;
        if (speed_pulse_count != UINT8_MAX) {
            ++speed_pulse_count;
        }
    }

    /* The Timer 1 prologue saved R0 from bank zero after selecting that bank. */
    __asm
        pop 0x00
        pop dph
        pop dpl
        pop b
        pop acc
        pop psw
    __endasm;
    if (timer1_overflow) {
        pending_timer1_overflow = 1;
    }
    __asm reti __endasm;
}
