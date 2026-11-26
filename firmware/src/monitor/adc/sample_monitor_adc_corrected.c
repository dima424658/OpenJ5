#include "monitor_adc.h"
#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;
static __sbit __at (0xd7) CY;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x44) uint8_t monitor_adc_minimum;
static volatile __data __at (0x45) uint8_t monitor_adc_maximum;
static volatile __data __at (0x46) uint8_t monitor_adc_mean_low;
static volatile __data __at (0x47) uint8_t monitor_adc_mean_high;
static volatile __data __at (0x48) uint8_t monitor_adc_span;
static volatile __xdata __at (0xff00) uint8_t monitor_eeprom_buffer[66];
static volatile __xdata __at (0xff30) uint8_t monitor_adc_corrections[15];

extern void monitor_eeprom_read_fragment(void);
extern void verify_monitor_word_checksum(void);
extern void select_monitor_adc_channel(void);
extern void capture_monitor_adc_samples(void);
extern void summarize_monitor_adc_samples(void);
extern void multiply_monitor_i8_u8(void);
extern void multiply_monitor_u16_u8(void);
extern void multiply_monitor_i16_u8(void);
extern void add_monitor_register_pairs(void);
extern void resume_service_monitor(void);

#define ADJUST_EXTREMUM(value)                          \
    do {                                                \
        ACC = stock_r4;                                 \
        B = (value);                                    \
        multiply_monitor_i8_u8();                       \
        ACC = B;                                        \
        B = 0x20;                                       \
        multiply_monitor_i8_u8();                       \
        const uint16_t adjusted = (uint16_t)(value) + B;\
        ACC = (B < 0x80 && adjusted > UINT8_MAX)        \
            ? UINT8_MAX : (uint8_t)adjusted;            \
        (value) = ACC;                                  \
    } while (0)

void sample_monitor_adc_corrected(void)
{
    stock_r0 = 0;
    STOCK_DPTR = (uint16_t)monitor_eeprom_buffer;
    stock_r1 = 66;
    monitor_eeprom_read_fragment();
    if (!CY) {
        STOCK_DPTR = (uint16_t)monitor_eeprom_buffer;
        stock_r2 = 33;
        verify_monitor_word_checksum();
    }
    if (CY) {
        monitor_checksum_failed = 1;
        monitor_command_active = 0;
        monitor_result_ready = 1;
        resume_service_monitor();
        return;
    }

    STOCK_DPTR = (uint16_t)monitor_adc_channels;
    select_monitor_adc_channel();
    capture_monitor_adc_samples();
    summarize_monitor_adc_samples();

    stock_r4 = monitor_adc_corrections[stock_r4];
    ADJUST_EXTREMUM(monitor_adc_minimum);
    ADJUST_EXTREMUM(monitor_adc_maximum);
    monitor_adc_span = monitor_adc_maximum - monitor_adc_minimum;

    stock_r0 = monitor_adc_mean_low;
    stock_r1 = monitor_adc_mean_high;
    B = stock_r4;
    multiply_monitor_u16_u8();
    if (stock_r4 & 0x80) {
        const uint8_t low = monitor_adc_mean_low;
        const uint8_t high = monitor_adc_mean_high;
        stock_r01 -= ((uint16_t)high << 8) | low;
    }
    B = 0x20;
    multiply_monitor_i16_u8();
    stock_r2 = monitor_adc_mean_low;
    stock_r3 = monitor_adc_mean_high;
    add_monitor_register_pairs();
    if (!(stock_r4 & 0x80) && CY) {
        stock_r01 = UINT16_MAX;
    }
    monitor_adc_mean_low = stock_r0;
    monitor_adc_mean_high = stock_r1;

    monitor_command_active = 0;
    monitor_result_ready = 1;
    resume_service_monitor();
}
