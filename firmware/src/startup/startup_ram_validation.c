#include "retained_ram.h"

enum {
    STARTUP_IAC_STATE_VALID = 0x55,
    RETAINED_RAM_CHECK_BYPASS = 0x02,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x16) retained_ram_checksum_failed;

static volatile __xdata __at (0xf4b8) uint8_t startup_state_f4b8;
static volatile __xdata __at (0xf800) uint8_t startup_ram_f800[256];
static volatile __xdata __at (0xf803) uint8_t startup_state_f803;
static volatile __xdata __at (0xf810) uint8_t startup_status_f810;
static volatile __xdata __at (0xf8c2) uint8_t startup_iac_state_marker;
static volatile __xdata __at (0xf900) uint8_t startup_ram_f900[52];
static volatile __xdata __at (0xf965) uint8_t startup_count_f965;
static volatile __xdata __at (0xf985) uint8_t startup_count_f985;
static volatile __xdata __at (0xf986) uint8_t idle_adaptation_count;
static volatile __xdata __at (0xf98b) uint8_t startup_state_f98b;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xff00) uint8_t startup_ram_ff00[128];
static volatile __xdata __at (0xff80) uint8_t startup_ram_ff80[4];

extern void subtract_u16_register_pairs(void);
extern void continue_startup_calibrations(void);

void validate_startup_ram(void);
void accept_startup_ram(void);
void clear_startup_ram(void);
void reset_startup_ram_fields(void);

void check_startup_ram(void)
{
    if (startup_iac_state_marker == STARTUP_IAC_STATE_VALID) {
        reset_startup_ram_fields();
        return;
    }
    if (retained_status & RETAINED_RAM_CHECK_BYPASS) {
        accept_startup_ram();
        return;
    }
    validate_startup_ram();
}

void validate_startup_ram(void)
{
    ACCUMULATE_RETAINED_RAM();
    stock_r23 = retained_ram_checksum;
    subtract_u16_register_pairs();
    if (stock_r01 != 0) {
        retained_ram_checksum_failed = 1;
        clear_startup_ram();
        return;
    }
    accept_startup_ram();
}

void accept_startup_ram(void)
{
    retained_ram_checksum_failed = 0;
    clear_startup_ram();
}

void clear_startup_ram(void)
{
    volatile __xdata uint8_t *entry = startup_ram_ff00;
    B = sizeof(startup_ram_ff00);
    do {
        *entry++ = 0;
    } while (--B != 0);

    entry = startup_ram_f900;
    B = sizeof(startup_ram_f900);
    do {
        *entry++ = 0;
    } while (--B != 0);

    entry = startup_ram_ff80;
    B = sizeof(startup_ram_ff80);
    do {
        *entry++ = 0;
    } while (--B != 0);

    entry = startup_ram_f800;
    do {
        *entry++ = 0;
    } while (entry != startup_ram_f800 + sizeof(startup_ram_f800));

    startup_state_f4b8 = 0;
    reset_startup_ram_fields();
}

void reset_startup_ram_fields(void)
{
    startup_count_f965 = 0;
    startup_count_f985 = 0;
    idle_adaptation_count = 0;
    startup_state_f98b = 0;
    startup_status_f810 = 0;
    startup_state_f803 = 0;
    continue_startup_calibrations();
}
