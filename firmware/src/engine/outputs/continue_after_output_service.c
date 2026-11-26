#include "stock_registers.h"

enum { ROUGHNESS_ALT_MAP = 1 };

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CY;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x59) idle_mode;

static volatile __data __at (0x00) uint16_t bank_zero_r01;
static volatile __data __at (0x4e) uint8_t coolant_axis_index;
static volatile __data __at (0x57) uint8_t rpm_index_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf93d) uint8_t primary_misfire_conditions;
static volatile __xdata __at (0xf942) uint8_t roughness_map_flags;
static volatile __xdata __at (0xf9da) uint8_t roughness_reduction_q8;
static volatile __xdata __at (0xf9e2) uint8_t roughness_sample_f9e2;
static volatile __xdata __at (0xf9ef) uint16_t roughness_threshold_1_q8;
static volatile __xdata __at (0xfa7e) uint8_t secondary_misfire_conditions;

extern __code const uint8_t roughness_idle_limit_1[32];
extern __code const uint8_t roughness_limit_1[256];
extern __code const uint8_t roughness_alt_limit_1[256];
extern __code const uint8_t roughness_temp_offset_1[39];
extern __code const uint8_t roughness_reduction_map[256];
extern __code const uint8_t roughness_secondary_curve[32];
extern volatile __code const uint8_t roughness_limit_1_drop;

extern void interp_curve_q4(void);
extern void interp_bilinear_u8(void);
extern void subtract_u16_register_pairs(void);
extern void update_roughness_base_limit_2(void);
extern void apply_roughness_base_limit_2(void);

void continue_after_output_service(void)
{
    if (idle_mode && !speed_sensor_fault && vehicle_speed == 0) {
        STOCK_DPTR = (uint16_t)roughness_idle_limit_1;
        stock_r2 = charge_q4;
        interp_curve_q4();
    } else {
        ACC = roughness_map_flags;
        STOCK_DPTR = (uint16_t)roughness_limit_1;
        if (ACC & ROUGHNESS_ALT_MAP) {
            STOCK_DPTR = (uint16_t)roughness_alt_limit_1;
        }
        stock_r2 = rpm_index_q4;
        stock_r3 = charge_q4;
        interp_bilinear_u8();
    }

    B = ACC;
    ACC = roughness_temp_offset_1[coolant_axis_index] + B;
    if (ACC < B) {
        ACC = UINT8_MAX;
    }
    B = ACC;
    stock_r01 = (uint16_t)ACC << 8;
    stock_r23 = roughness_threshold_1_q8;
    subtract_u16_register_pairs();
    if (CY) {
        // The stock restore writes to physical IRAM:00–01.
        bank_zero_r01 = stock_r23;
        stock_r23 = roughness_limit_1_drop;
        subtract_u16_register_pairs();
    } else {
        stock_r01 = (uint16_t)B << 8;
    }
    roughness_threshold_1_q8 = stock_r01;

    B = stock_r1;
    if (roughness_sample_f9e2 <= B) {
        ACC = UINT8_MAX;
    } else {
        STOCK_DPTR = (uint16_t)roughness_reduction_map;
        stock_r2 = rpm_index_q4;
        stock_r3 = charge_q4;
        interp_bilinear_u8();
    }
    roughness_reduction_q8 = ACC;

    if ((secondary_misfire_conditions & 0x80) && (primary_misfire_conditions & 0x80)) {
        STOCK_DPTR = (uint16_t)roughness_secondary_curve;
        stock_r2 = rpm_index_q4;
        interp_curve_q4();
        stock_r01 = (uint16_t)ACC << 8;
        apply_roughness_base_limit_2();
        return;
    }
    update_roughness_base_limit_2();
}
