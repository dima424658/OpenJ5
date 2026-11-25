#include <stdint.h>

/* Mixture target when throttle or MAF input has failed. */
volatile __code const uint8_t sensor_fault_mixture_raw = 128;
