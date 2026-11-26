#include "stock_registers.h"

static __sfr __at (0x8a) TL0;
static __sfr __at (0xf0) B;
static __sfr __at (0xf2) sfr_f2;
static __sfr __at (0xf3) sfr_f3;
static __sfr __at (0xf6) sfr_f6;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe6) ACC_BIT6;
static __sbit __at (0xf1) B_BIT1;
static __sbit __at (0xf2) B_BIT2;

static __sbit __at (0x15) startup_mode_15;
static __sbit __at (0x16) retained_ram_checksum_failed;
static __sbit __at (0x51) startup_flag_51;
static __sbit __at (0x53) startup_flag_53;
static __sbit __at (0x54) startup_flag_54;
static __sbit __at (0x58) startup_flag_58;
static __sbit __at (0x5f) startup_flag_5f;
static __sbit __at (0x62) idle_transition_hold;
static __sbit __at (0x70) startup_flag_70;

static volatile __data __at (0x60) uint8_t fuel_mixture_target;
static volatile __data __at (0x61) uint8_t fuel_mixture_current;
static volatile __data __at (0x6e) uint8_t startup_state_6e;
static volatile __data __at (0x6f) uint8_t injection_time_correction;
static volatile __idata __at (0xbe) uint8_t startup_state_be;

static volatile __xdata __at (0xf806) uint8_t state_f806;
static volatile __xdata __at (0xf827) uint8_t throttle_zero_reference;
static volatile __xdata __at (0xf830) uint8_t startup_state_f830;
static volatile __xdata __at (0xf831) uint8_t startup_state_f831;
static volatile __xdata __at (0xf835) uint8_t startup_state_f835;
static volatile __xdata __at (0xf843) uint8_t startup_state_f843;
static volatile __xdata __at (0xf866) uint8_t startup_state_f866;
static volatile __xdata __at (0xf867) uint8_t startup_state_f867;
static volatile __xdata __at (0xf89e) uint8_t startup_state_f89e;
static volatile __xdata __at (0xf8b2) uint8_t throttle_position_baseline;
static volatile __xdata __at (0xf97f) uint8_t fuel_weighted_correction;
static volatile __xdata __at (0xf9ad) uint8_t retained_flags;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xffc0) uint8_t retained_fuel_correction;

extern volatile __code const uint8_t ecu_configuration_flags_1;
extern volatile __code const uint8_t maf_min_diagnostic_flow;
extern volatile __code const uint8_t startup_mixture;
extern volatile __code const uint8_t startup_throttle_zero;
extern volatile __code const uint8_t startup_injection_correction;

extern void reset_start_mode_3edb(void);
extern void reset_fuel_correction_map(void);
extern void continue_startup_fuel_trim(void);
void configure_startup_output_bits(void);

void init_startup_control_state(void)
{
    TL0 = maf_min_diagnostic_flow;
    startup_state_be = 0xb0;
    TL0 = maf_min_diagnostic_flow;

    ACC = 0;
    startup_state_f866 = ACC;
    startup_state_f867 = ACC;
    ACC = UINT8_MAX;
    startup_state_f830 = ACC;
    startup_state_f831 = ACC;
    ACC = 0x20;
    startup_state_f843 = ACC;

    ACC = startup_mixture;
    fuel_mixture_target = ACC;
    fuel_mixture_current = ACC;
    ACC = UINT8_MAX;
    startup_state_f835 = ACC;
    ACC = startup_throttle_zero;
    throttle_position_baseline = ACC;
    throttle_zero_reference = ACC;
    startup_state_6e = 0;
    injection_time_correction = startup_injection_correction;
    ACC = 0;
    startup_state_f89e = ACC;
    startup_flag_70 = 1;

    ACC = 0;
    state_f806 = ACC;
    sfr_f3 = ACC;
    sfr_f2 = 0x0f;
    B = ecu_configuration_flags_1;
    configure_startup_output_bits();
}

void configure_startup_output_bits(void)
{
    ACC_BIT6 = B_BIT1;
    ACC_SIGN = B_BIT2;
    sfr_f6 = ACC & 0xc0;
    startup_flag_58 = 1;
    startup_flag_53 = 1;
    startup_flag_54 = 1;

    ACC = retained_status;
    if (!startup_mode_15 && ACC_BIT1 && retained_ram_checksum_failed) {
        idle_transition_hold = 1;
        retained_flags = 0;
    }
    startup_flag_51 = 1;
    startup_flag_5f = 0;
    reset_start_mode_3edb();

    ACC = retained_status;
    if (startup_mode_15 || ACC_BIT1 || retained_ram_checksum_failed) {
        reset_fuel_correction_map();
        ACC = 0x80;
        retained_fuel_correction = ACC;
        fuel_weighted_correction = ACC;
    }
    continue_startup_fuel_trim();
}
