#include <stdint.h>

/* Upper startup coolant threshold in the stock temperature encoding (80 °C). */
volatile __code const uint8_t dew_point_coolant_high = 120;
