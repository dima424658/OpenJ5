#include "stock_registers.h"

enum {
    COOLANT_ADC_CHANNEL = 4,
    COOLANT_STARTUP_OFFSET = 20,
    COOLANT_AXIS_STEP = 5,
    COOLANT_AXIS_LIMIT = 36,
};

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __xdata __at (0xf80b) uint8_t coolant_adc_raw;
static volatile __xdata __at (0xf9b5) uint8_t startup_coolant_raw;

extern __code const uint8_t coolant_transfer_raw[32];
extern void read_adc_8bit(void);
extern void interp_curve_u8(void);
extern void set_warmup_duration(void);

void sample_startup_coolant(void)
{
    B = COOLANT_ADC_CHANNEL;
    read_adc_8bit();
    coolant_adc_raw = ACC;
    stock_r1 = ACC;
    stock_r3 = (uint8_t)(stock_r1 << 5);
    stock_r2 = stock_r1 >> 3;
    STOCK_DPTR = (uint16_t)coolant_transfer_raw;
    interp_curve_u8();
    ACC = ACC < COOLANT_STARTUP_OFFSET ? 0 : ACC - COOLANT_STARTUP_OFFSET;
    coolant_temperature_raw = ACC;
    startup_coolant_raw = ACC;

    ACC = (uint8_t)(coolant_temperature_raw + COOLANT_AXIS_STEP / 2) / COOLANT_AXIS_STEP;
    if (ACC >= COOLANT_AXIS_LIMIT) {
        ACC = COOLANT_AXIS_LIMIT;
    }
    coolant_nearest_index = ACC;
    set_warmup_duration();
}
