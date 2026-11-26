#include <stdint.h>

enum {
    IGNITION_SUPPLY_ADC_CHANNEL = 1,
    SHUTDOWN_DELAY_INHIBIT = 0x08,
    SHUTDOWN_IMMEDIATE_MODE = 2,
};

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0xaf) interrupts_enabled;

static __sbit __at (0x49) tick_1s;
static __sbit __at (0x4c) ignition_supply_low;
static __sbit __at (0x4d) shutdown_pending;
static __sbit __at (0x53) condition_flag_53;
static __sbit __at (0x62) idle_transition_hold;
static __sbit __at (0x68) shutdown_serial_mode;
static volatile __data __at (0x00) uint8_t bank_zero_supply_threshold;
static volatile __data __at (0x2f) uint8_t status_2f;

static volatile __xdata __at (0xf812) uint8_t sensor_sample_count;
static volatile __xdata __at (0xf47d) uint8_t shutdown_options;
static volatile __xdata __at (0xf4b7) uint8_t shutdown_mode;
static volatile __xdata __at (0xf807) uint8_t elapsed_seconds_f807;
static volatile __xdata __at (0xf87c) uint8_t shutdown_elapsed;
static volatile __xdata __at (0xf9be) uint8_t elapsed_seconds_f9be;
static volatile __xdata __at (0xfa76) uint8_t shutdown_service_countdown;

extern volatile __code const uint8_t ignition_key_analysis_ticks;
extern volatile __code const uint8_t shutdown_service_initial;
extern volatile __code const uint8_t shutdown_delay_raw;
extern volatile __code const uint8_t shutdown_voltage_raw;

extern void clear_sensor_sample_sums(void);
extern void stop_engine_outputs(void);
extern void update_relay_outputs(void);
extern void park_iac_at_shutdown(void);
extern void read_adc_8bit(void);
extern void initialize_serial_interface(void);
extern void mix_shutdown_timer_samples(void);
extern void continue_engine_stop_cycle(void);
extern void save_shutdown_state(void);

void dispatch_engine_shutdown(void);
void clear_shutdown_request(void);
void begin_ignition_shutdown(void);
void read_supply_during_shutdown(void);
void service_pending_shutdown(void);
void begin_power_down(void);

void update_engine_stop_counters(void)
{
    if (tick_1s) {
        ACC = elapsed_seconds_f807 + 1;
        if (ACC != 0) {
            elapsed_seconds_f807 = ACC;
        }
    }
    if (tick_1s && !condition_flag_53) {
        ACC = elapsed_seconds_f9be + 1;
        if (ACC != 0) {
            elapsed_seconds_f9be = ACC;
        }
    }
    dispatch_engine_shutdown();
}

void dispatch_engine_shutdown(void)
{
    if (shutdown_pending) {
        service_pending_shutdown();
        return;
    }
    if (ignition_supply_low) {
        begin_ignition_shutdown();
        return;
    }
    clear_shutdown_request();
}

void reset_engine_stop_counters(void)
{
    clear_sensor_sample_sums();
    sensor_sample_count = 0;
    elapsed_seconds_f807 = 0;
    elapsed_seconds_f9be = 0;
    idle_transition_hold = 1;
    clear_shutdown_request();
}

void clear_shutdown_request(void)
{
    shutdown_elapsed = 0;
    shutdown_pending = 0;
    continue_engine_stop_cycle();
}

void begin_ignition_shutdown(void)
{
    B = ignition_key_analysis_ticks;
    ACC = shutdown_elapsed + 1;
    shutdown_elapsed = ACC;
    if (ACC >= B) {
        stop_engine_outputs();
        update_relay_outputs();
        shutdown_service_countdown = shutdown_service_initial;
        shutdown_elapsed = 0;
        shutdown_pending = 1;
        park_iac_at_shutdown();
    }
    read_supply_during_shutdown();
}

void read_supply_during_shutdown(void)
{
    bank_zero_supply_threshold = shutdown_voltage_raw;
    B = IGNITION_SUPPLY_ADC_CHANNEL;
    read_adc_8bit();
    if (ACC >= bank_zero_supply_threshold) {
        begin_power_down();
        return;
    }
    continue_engine_stop_cycle();
}

void service_pending_shutdown(void)
{
    if (!ignition_supply_low) {
        goto read_supply;
    }
    ACC = shutdown_service_countdown - 1;
    if (ACC != 0) {
        shutdown_service_countdown = ACC;
        goto read_supply;
    }
    if (shutdown_options & SHUTDOWN_DELAY_INHIBIT) {
        goto read_supply;
    }
    if (shutdown_mode == SHUTDOWN_IMMEDIATE_MODE) {
        begin_power_down();
        return;
    }

    B = shutdown_delay_raw;
    ACC = shutdown_elapsed + 1;
    shutdown_elapsed = ACC;
    if (ACC < B) {
        goto read_supply;
    }
    shutdown_options |= SHUTDOWN_DELAY_INHIBIT;
    status_2f = 0;
    shutdown_serial_mode = 1;
    initialize_serial_interface();
    mix_shutdown_timer_samples();
    continue_engine_stop_cycle();
    return;

read_supply:
    read_supply_during_shutdown();
}

void begin_power_down(void)
{
    interrupts_enabled = 0;
    save_shutdown_state();
}
