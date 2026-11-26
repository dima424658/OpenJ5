#include <stdint.h>

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;

static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __xdata __at (0xf93e) uint8_t misfire_summary;
static volatile __xdata __at (0xf940) uint8_t misfire_rpm_bin_offset;
static volatile __xdata __at (0xf941) uint8_t misfire_rpm_bin_mask;
static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xf94e) uint8_t misfire_initial_threshold_1;
static volatile __xdata __at (0xf94f) uint8_t misfire_initial_threshold_2;

extern __code const uint8_t misfire_rpm_boundaries[5];
extern __code const uint8_t misfire_threshold_1_by_rpm[32];
extern __code const uint8_t misfire_threshold_2_by_rpm[32];
extern void select_misfire_threshold(void);

void publish_misfire_rpm_bin(void);
void load_misfire_thresholds(void);

void knock_zone_update_continued(void)
{
    /* Each bin has a byte offset into a word table and a separate status mask. */
    if (misfire_rpm_boundaries[0] >= rpm_eighth) {
        B = 0;
        ACC = 0;
    } else if (misfire_rpm_boundaries[1] >= rpm_eighth) {
        B = 0x01;
        ACC = 2;
    } else if (misfire_rpm_boundaries[2] >= rpm_eighth) {
        B = 0x02;
        ACC = 4;
    } else if (misfire_rpm_boundaries[3] >= rpm_eighth) {
        B = 0x04;
        ACC = 6;
    } else if (misfire_rpm_boundaries[4] >= rpm_eighth) {
        B = 0x08;
        ACC = 8;
    } else {
        ACC = 10;
        B = 0x10;
    }

    publish_misfire_rpm_bin();
}

void publish_misfire_rpm_bin(void)
{
    misfire_rpm_bin_offset = ACC;
    ACC = B;
    misfire_rpm_bin_mask = ACC;
    ACC &= 0x0f;
    if (ACC != 0) {
        ACC = misfire_status;
        if (ACC_BIT0 && ACC_BIT1 && ACC_BIT2 && ACC_BIT3) {
            ACC = misfire_summary;
            ACC_BIT0 = 1;
            goto publish;
        }
    }

    ACC = misfire_summary;
    ACC_BIT0 = 0;

publish:
    misfire_summary = ACC;
    load_misfire_thresholds();
}

void load_misfire_thresholds(void)
{
    misfire_initial_threshold_1 = misfire_threshold_1_by_rpm[rpm_nearest_half_bin];
    misfire_initial_threshold_2 = misfire_threshold_2_by_rpm[rpm_nearest_half_bin];
    select_misfire_threshold();
}
