#include <stdint.h>

#define COOLANT_C_TO_CODE(celsius) ((uint8_t)((celsius) + 40))

__code const uint8_t fan_off_temperature = COOLANT_C_TO_CODE(98);
__code const uint8_t fan_on_temperature = COOLANT_C_TO_CODE(101);
