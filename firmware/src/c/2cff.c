#include <stdint.h>

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;
static __sbit __at (0xf1) B_BIT1;
static __sbit __at (0xf2) B_BIT2;
static __sbit __at (0xf3) B_BIT3;

static volatile __xdata __at (0xf941) uint8_t misfire_rpm_bin_mask;
static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xf94d) uint8_t misfire_threshold;
static volatile __xdata __at (0xf94e) uint8_t misfire_initial_threshold_1;
static volatile __xdata __at (0xf94f) uint8_t misfire_initial_threshold_2;
static volatile __xdata __at (0xfa43) uint16_t retained_misfire_words[4];
static volatile __xdata __at (0xfa53) uint16_t active_misfire_words[4];

extern void continue_misfire_rpm(void);
void load_active_misfire_words(void);

void select_misfire_threshold(void)
{
    ACC = misfire_status;
    B = ACC;
    if (!ACC_BIT0) {
        ACC = misfire_initial_threshold_2;
        misfire_threshold = ACC;
        goto publish;
    }

    ACC = misfire_rpm_bin_mask & 0x1f;
    if (ACC == 0 || ACC_BIT0) {
        ACC = 0;
    } else if (ACC_BIT1) {
        if (B_BIT1)
            ACC = 0;
        else
            ACC = misfire_initial_threshold_1;
    } else if (ACC_BIT2) {
        if (B_BIT2)
            ACC = 0;
        else
            ACC = misfire_initial_threshold_1;
    } else if (ACC_BIT3) {
        if (B_BIT3)
            ACC = 0;
        else
            ACC = misfire_initial_threshold_1;
    } else {
        /* The last two RPM ranges use the same status bit. */
        if (B_BIT3)
            ACC = 0;
        else
            ACC = misfire_initial_threshold_1;
    }

publish:
    misfire_threshold = ACC;
    load_active_misfire_words();
}

void load_active_misfire_words(void)
{
    ACC = misfire_status;
    if (!ACC_BIT0) {
        volatile __xdata uint8_t *bytes = (volatile __xdata uint8_t *)active_misfire_words;
        bytes[0] = 0;
        bytes[2] = 0;
        bytes[4] = 0;
        bytes[6] = 0;
        bytes[1] = 0x80;
        bytes[3] = 0x80;
        bytes[5] = 0x80;
        bytes[7] = 0x80;
        continue_misfire_rpm();
        return;
    }

    active_misfire_words[0] = retained_misfire_words[0];
    ACC = misfire_status;
    active_misfire_words[1] = ACC_BIT1 ? retained_misfire_words[1] : retained_misfire_words[0];
    ACC = misfire_status;
    active_misfire_words[2] = ACC_BIT2 ? retained_misfire_words[2] : retained_misfire_words[0];
    ACC = misfire_status;
    active_misfire_words[3] = ACC_BIT3 ? retained_misfire_words[3] : retained_misfire_words[0];
    continue_misfire_rpm();
}
