#include <stdint.h>

enum { FAN_OVERRIDE = 0x02 };

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe1) ACC_RELAY;
static __sbit __at (0x1b) coolant_fault_1b;
static __sbit __at (0x23) coolant_fault_23;
static __sbit __at (0x6e) fan_aux_request_6e;
static __sbit __at (0x71) fan_override_on;
static __sbit __at (0x41) fan_off;
static __sbit __at (0x59) idle_mode;
static volatile __data __at (0x79) uint8_t fan_control_79;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x64) uint8_t iac_target_raw;
static volatile __xdata __at (0xf8bf) uint8_t fan_start_counter;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;

extern __code const uint8_t fan_off_temperature;
extern __code const uint8_t fan_on_temperature;
extern volatile __code const uint8_t fan_iac_offset;
extern volatile __code const uint8_t fan_start_delay_ticks;
extern void clear_iac_history(void);
extern void cooling_fan_continue(void);

// The stock blocks share the request in CY and the counter in ACC.
void apply_fan_delay(void);
void publish_fan_request(void);

void update_cooling_fan(void)
{
    const uint8_t override = fan_control_79 & FAN_OVERRIDE;
    if (override) {
        // Forced operation stores the override mask as the counter.
        ACC = override;
        CY = fan_override_on;
        publish_fan_request();
        return;
    }

    CY = coolant_fault_1b;
    CY |= coolant_fault_23;
    CY |= fan_aux_request_6e;
    if (!CY) {
        if (fan_off) {
            B = fan_on_temperature;
        } else {
            B = fan_off_temperature;
        }
        CY = coolant_temperature_raw >= B;
    }
    apply_fan_delay();
}

void apply_fan_delay(void)
{
    if (idle_mode && fan_off && CY) {
        B = fan_start_delay_ticks;
        uint8_t counter = fan_start_counter;
        if (counter == 0) {
            iac_target_raw += fan_iac_offset;
            clear_iac_history();
            counter = 0;
        }
        ++counter;
        CY = counter >= B;
        if (!CY) {
            ACC = counter;
            publish_fan_request();
            return;
        }
    }

    ACC = 0;
    publish_fan_request();
}

void publish_fan_request(void)
{
    fan_start_counter = ACC;
    fan_off = !CY;
    CY = !fan_off;
    ACC = relay_output_latch;
    ACC_RELAY = CY;
    relay_output_latch = ACC;
    cooling_fan_continue();
}
