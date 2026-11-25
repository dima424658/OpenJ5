#include "stock_registers.h"

static __sfr __at (0xf0) B;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (0x61) uint8_t alternate_startup_marker;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;

#define STARTUP_ADC_OUTSIDE(channel, low, high) \
    (B = (channel), read_adc_8bit(), stock_r0 = ACC, \
     stock_r0 < (low) || stock_r0 >= (high))

extern void read_adc_8bit(void);
extern void enter_alternate_startup(void);
extern void read_eeprom_record_f485(void);
extern void read_eeprom_record_f48d(void);
extern void read_eeprom_record_f495(void);
extern void read_eeprom_record_f460(void);
extern void read_eeprom_record_f474(void);
extern void read_eeprom_record_f480(void);
extern void check_eeprom_record_9d(void);
extern void restore_startup_eeprom_bd12(void);
extern void restore_startup_eeprom_c470(void);
extern void restart_runtime_init(void);

void continue_startup_sensor_test(void)
{
    *((volatile __idata uint8_t *)stock_r0) = 0;
    alternate_startup_marker = 0;

    /* Each range includes its lower bound and excludes its upper bound. */
    if (STARTUP_ADC_OUTSIDE(0, 0xdd, 0xe8) ||
        STARTUP_ADC_OUTSIDE(13, 0xf5, 0xfe) ||
        STARTUP_ADC_OUTSIDE(4, 0xf5, 0xfe) ||
        STARTUP_ADC_OUTSIDE(3, 0x15, 0x1e) ||
        STARTUP_ADC_OUTSIDE(11, 0xf5, 0xfe)) {
        read_eeprom_record_f485();
        read_eeprom_record_f48d();
        read_eeprom_record_f495();
        read_eeprom_record_f460();
        read_eeprom_record_f474();
        read_eeprom_record_f480();
        check_eeprom_record_9d();
        if (stock_r5 != 0x55 || (retained_status & 0x02)) {
            restore_startup_eeprom_bd12();
        }
        restore_startup_eeprom_c470();
        restart_runtime_init();
        return;
    }

    alternate_startup_marker = 0x55;
    enter_alternate_startup();
}
