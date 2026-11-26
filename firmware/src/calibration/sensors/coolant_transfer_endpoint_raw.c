#include <stdint.h>

/* The fractional lookup at the last coolant ADC bin can read this sample. */
__code const uint8_t coolant_transfer_endpoint_raw = 10;

/* No confirmed runtime reference to the following bytes. */
__code const uint8_t calibration_6125[18] = {
    5, 244, 196, 167, 151, 140, 131, 123, 116,
    109, 103, 97, 90, 83, 75, 65, 50, 15,
};
