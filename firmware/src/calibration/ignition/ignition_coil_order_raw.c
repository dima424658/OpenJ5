#include <stdint.h>

/* Selects the order of the ignition coils through bit zero. */
volatile __code const uint8_t ignition_coil_order_raw = 1;
