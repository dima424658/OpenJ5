#include <stdint.h>

enum { SENSOR_TICKS_PER_SECOND = 50, SECONDS_PER_MINUTE = 60 };

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0x15) startup_mode_15;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x4a) tick_1min;
static __sbit __at (0x4f) oxygen_threshold_set_4f;
static __sbit __at (0x58) crank_stopped;
static __sbit __at (0x6e) air_conditioning_active;

static volatile __xdata __at (0xf804) uint8_t sensor_ticks_in_second;
static volatile __xdata __at (0xf805) uint8_t throttle_zero_drift_timer;
static volatile __xdata __at (0xf84f) uint8_t idle_rpm_correction;
static volatile __xdata __at (0xf898) uint8_t sensor_tick_sequence;
static volatile __xdata __at (0xf8ac) uint8_t fallback_runtime;
static volatile __xdata __at (0xf965) uint8_t startup_count_f965;

extern volatile __code const uint8_t ac_idle_rpm_offset_min;
extern volatile __code const uint8_t ac_idle_rpm_offset_decay;

extern void service_second_event_ad95(void);
extern void update_engine_stop_counters(void);

void finish_sensor_diagnostics(void)
{
    tick_1s = 0;
    tick_1min = 0;
    ++sensor_tick_sequence;

    ACC = sensor_ticks_in_second + 1;
    sensor_ticks_in_second = ACC;
    if (ACC == SENSOR_TICKS_PER_SECOND) {
        sensor_ticks_in_second = 0;
        if (oxygen_threshold_set_4f) {
            service_second_event_ad95();
        }
        tick_1s = 1;

        if (!crank_stopped) {
            ACC = fallback_runtime + 1;
            if (ACC != 0) {
                fallback_runtime = ACC;
            }
        }
        ACC = startup_count_f965 + 1;
        if (ACC != 0) {
            startup_count_f965 = ACC;
        }

        if (air_conditioning_active) {
            B = ac_idle_rpm_offset_decay;
            const uint8_t offset = idle_rpm_correction;
            B = offset >= B ? offset - B : 0;
            ACC = ac_idle_rpm_offset_min;
            if (ACC < B) {
                ACC = B;
            }
            idle_rpm_correction = ACC;
        }

        ACC = throttle_zero_drift_timer + 1;
        throttle_zero_drift_timer = ACC;
        if (ACC == SECONDS_PER_MINUTE) {
            throttle_zero_drift_timer = 0;
            tick_1min = 1;
            startup_mode_15 = 0;
        }
    }
    update_engine_stop_counters();
}
