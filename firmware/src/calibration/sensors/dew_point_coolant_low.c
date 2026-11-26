#include <stdint.h>

/* Lower startup coolant threshold in the stock temperature encoding (15 °C). */
volatile __code const uint8_t dew_point_coolant_low = 55;
