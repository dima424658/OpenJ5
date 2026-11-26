#include "stock_registers.h"

enum {
    INITIAL_KNOCK_COUNT = 5,
    INITIAL_ATTENUATOR_STATE = 8,
    ATTENUATOR_STATE_LIMIT = 24,
};

static __sfr __at (0xf0) B;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0x15) startup_mode_15;
static __sbit __at (0x16) retained_ram_checksum_failed;
static volatile __data __at (0x47) uint8_t noise_attenuator_state;
static volatile __xdata __at (0xf934) uint8_t knock_noise_filter_low;
static volatile __xdata __at (0xf935) uint8_t knock_noise_filter_high;
static volatile __xdata __at (0xf936) uint8_t knock_average_noise;
static volatile __xdata __at (0xf943) uint8_t knock_threshold;
static volatile __xdata __at (0xf946) uint8_t knock_window_start;
static volatile __xdata __at (0xf947) uint8_t knock_window_end;
static volatile __xdata __at (0xf953) uint8_t knock_min_interval;
static volatile __xdata __at (0xf954) uint8_t advance_recovery_period;
static volatile __xdata __at (0xf955) uint8_t knock_cylinder_count[4];
static volatile __xdata __at (0xf966) uint8_t knock_cylinder_filter[8];
static volatile __xdata __at (0xf96e) uint8_t knock_cylinder_noise[4];
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xffce) uint8_t retained_knock_noise;
static volatile __xdata __at (0xffcf) uint8_t retained_attenuator_state;

extern volatile __code const uint8_t startup_knock_min_interval;
extern volatile __code const uint8_t startup_advance_period;
extern volatile __code const uint8_t startup_knock_threshold;
extern volatile __code const uint8_t startup_knock_window_start;
extern volatile __code const uint8_t startup_knock_window_end;
extern volatile __code const uint8_t startup_knock_average_noise;
extern volatile __code const uint8_t retained_noise_min_exclusive;

extern void continue_startup_attenuation(void);

void select_startup_knock_noise(void);
void restore_startup_knock_noise(void);
void reset_retained_knock_noise(void);
void write_startup_attenuator_state(void);
void seed_startup_knock_filters(void);

/* Both paths copy the starting noise into the same five locations. */
#define SET_STARTUP_KNOCK_NOISE()         \
    do {                                  \
        knock_average_noise = ACC;        \
        knock_cylinder_noise[0] = ACC;    \
        knock_cylinder_noise[1] = ACC;    \
        knock_cylinder_noise[2] = ACC;    \
        knock_cylinder_noise[3] = ACC;    \
    } while (0)

void continue_startup_calibrations(void)
{
    knock_min_interval = startup_knock_min_interval;
    advance_recovery_period = startup_advance_period;
    knock_threshold = startup_knock_threshold;
    knock_window_start = startup_knock_window_start;
    knock_window_end = startup_knock_window_end;
    ACC = INITIAL_KNOCK_COUNT;
    knock_cylinder_count[0] = ACC;
    knock_cylinder_count[1] = ACC;
    knock_cylinder_count[2] = ACC;
    knock_cylinder_count[3] = ACC;
    select_startup_knock_noise();
}

void select_startup_knock_noise(void)
{
    ACC = retained_status;
    if ((retained_ram_checksum_failed || ACC_BIT1) && !startup_mode_15) {
        ACC = startup_knock_average_noise;
        SET_STARTUP_KNOCK_NOISE();
        retained_knock_noise = ACC;
        noise_attenuator_state = INITIAL_ATTENUATOR_STATE;
        /* This branch leaves the initial noise byte in the retained slot. */
        retained_attenuator_state = ACC;
        seed_startup_knock_filters();
        return;
    }
    restore_startup_knock_noise();
}

void restore_startup_knock_noise(void)
{
    B = retained_knock_noise;
    if ((uint8_t)(startup_knock_average_noise * 2u) < B ||
        retained_noise_min_exclusive >= B) {
        goto reset_noise;
    }

    ACC = B;
    SET_STARTUP_KNOCK_NOISE();
    ACC = retained_attenuator_state;
    if (ACC < ATTENUATOR_STATE_LIMIT) {
        write_startup_attenuator_state();
        return;
    }

reset_noise:
    reset_retained_knock_noise();
}

void reset_retained_knock_noise(void)
{
    retained_knock_noise = startup_knock_average_noise;
    ACC = INITIAL_ATTENUATOR_STATE;
    write_startup_attenuator_state();
}

void write_startup_attenuator_state(void)
{
    noise_attenuator_state = ACC;
    retained_attenuator_state = ACC;
    seed_startup_knock_filters();
}

void seed_startup_knock_filters(void)
{
    ACC = retained_knock_noise >> 1;
    knock_noise_filter_high = ACC;
    knock_cylinder_filter[1] = ACC;
    knock_cylinder_filter[3] = ACC;
    knock_cylinder_filter[5] = ACC;
    knock_cylinder_filter[7] = ACC;

    ACC = 0;
    knock_noise_filter_low = ACC;
    knock_cylinder_filter[0] = ACC;
    knock_cylinder_filter[2] = ACC;
    knock_cylinder_filter[4] = ACC;
    knock_cylinder_filter[6] = ACC;
    continue_startup_attenuation();
}
