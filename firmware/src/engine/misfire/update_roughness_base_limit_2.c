#include "stock_registers.h"

enum { ROUGHNESS_ALT_MAP = 0x01, RETAINED_TICK_MASK = 0x01 };

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x48) timer_event_48;
static __sbit __at (0x59) idle_mode;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x00) uint16_t bank_zero_r01;
static volatile __data __at (0x4e) uint8_t coolant_axis_index;
static volatile __data __at (0x57) uint8_t rpm_index_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf942) uint8_t roughness_map_flags;
static volatile __xdata __at (0xf94d) uint8_t misfire_threshold;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xf9d9) uint8_t roughness_base_2;
static volatile __xdata __at (0xf9da) uint8_t roughness_reduction_q8;
static volatile __xdata __at (0xf9db) uint16_t roughness_threshold_2_q8;
static volatile __xdata __at (0xf9f6) uint16_t roughness_reduced_2_q8;
static volatile __xdata __at (0xf9f8) uint16_t roughness_applied_2_q8;
static volatile __xdata __at (0xf9fa) uint16_t roughness_threshold_3_q8;
static volatile __xdata __at (0xfa08) uint8_t roughness_base_3;
static volatile __xdata __at (0xfa7a) uint16_t roughness_3_subtraction;
static volatile __xdata __at (0xfa7c) uint16_t roughness_unadjusted_3_q8;

extern __code const uint8_t roughness_idle_limit_2[32];
extern __code const uint8_t roughness_limit_2[256];
extern __code const uint8_t roughness_alt_limit_2[256];
extern __code const uint8_t roughness_temp_offset_2[39];
extern volatile __code const uint8_t roughness_limit_2_drop;
extern __code const uint8_t roughness_idle_limit_3[32];
extern __code const uint8_t roughness_limit_3[256];
extern __code const uint8_t roughness_alt_limit_3[256];
extern __code const uint8_t roughness_temp_offset_3[39];
extern volatile __code const uint8_t roughness_limit_3_drop;

extern void interp_curve_q4(void);
extern void interp_bilinear_u8(void);
extern void subtract_u16_register_pairs(void);
extern void multiply_stock_u16_u8(void);
extern void wait_for_control_tick(void);
void apply_roughness_base_limit_2(void);

#define LOOK_UP_ROUGHNESS(idle, regular, alternate)                          \
    do {                                                                    \
        if (idle_mode && !speed_sensor_fault && vehicle_speed == 0) {        \
            STOCK_DPTR = (uint16_t)(idle);                                   \
            stock_r2 = charge_q4;                                            \
            interp_curve_q4();                                              \
        } else {                                                            \
            ACC = roughness_map_flags;                                      \
            STOCK_DPTR = (uint16_t)(regular);                                \
            if (ACC & ROUGHNESS_ALT_MAP) {                                   \
                STOCK_DPTR = (uint16_t)(alternate);                          \
            }                                                               \
            stock_r2 = rpm_index_q4;                                         \
            stock_r3 = charge_q4;                                            \
            interp_bilinear_u8();                                            \
        }                                                                   \
    } while (0)

#define APPROACH_ROUGHNESS_TARGET(offset, previous, drop, after_drop)     \
    do {                                                                    \
        B = ACC;                                                            \
        ACC = (offset)[coolant_axis_index] + B;                               \
        if (ACC < B) {                                                      \
            ACC = UINT8_MAX;                                                 \
        }                                                                   \
        B = ACC;                                                            \
        stock_r01 = (uint16_t)ACC << 8;                                      \
        stock_r23 = (previous);                                             \
        subtract_u16_register_pairs();                                      \
        if (CY) {                                                           \
            /* The stock restore targets physical IRAM:00–01. */            \
            bank_zero_r01 = stock_r23;                                       \
            stock_r23 = (drop);                                              \
            subtract_u16_register_pairs();                                  \
            after_drop;                                                      \
        } else {                                                            \
            stock_r01 = (uint16_t)B << 8;                                    \
        }                                                                   \
    } while (0)

#define CLAMP_STOCK_WORD_AT_ZERO() \
    do {                           \
        if (CY) {                  \
            stock_r01 = 0;         \
        }                          \
    } while (0)

void update_roughness_base_limit_2(void)
{
    LOOK_UP_ROUGHNESS(roughness_idle_limit_2, roughness_limit_2, roughness_alt_limit_2);
    roughness_base_2 = ACC;
    APPROACH_ROUGHNESS_TARGET(roughness_temp_offset_2, roughness_threshold_2_q8,
                              roughness_limit_2_drop, CLAMP_STOCK_WORD_AT_ZERO());
    roughness_threshold_2_q8 = stock_r01;

    B = roughness_reduction_q8;
    multiply_stock_u16_u8();
    roughness_reduced_2_q8 = stock_r01;
    B = misfire_threshold;
    if (stock_r1 < B) {
        stock_r01 = (uint16_t)B << 8;
    }
    apply_roughness_base_limit_2();
}

void apply_roughness_base_limit_2(void)
{
    roughness_applied_2_q8 = stock_r01;
    LOOK_UP_ROUGHNESS(roughness_idle_limit_3, roughness_limit_3, roughness_alt_limit_3);
    roughness_base_3 = ACC;
    APPROACH_ROUGHNESS_TARGET(roughness_temp_offset_3, roughness_threshold_3_q8,
                              roughness_limit_3_drop, (void)0);
    roughness_unadjusted_3_q8 = stock_r01;
    stock_r23 = roughness_3_subtraction;
    subtract_u16_register_pairs();
    if (CY) {
        stock_r01 = 0;
    }
    roughness_threshold_3_q8 = stock_r01;

    ACC = retained_status & ~RETAINED_TICK_MASK;
    if (timer_event_48) {
        ACC |= RETAINED_TICK_MASK;
    }
    retained_status = ACC;
    wait_for_control_tick();
}
