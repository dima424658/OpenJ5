#include <stdint.h>

/* Largest encoded ALF value allowed at each coolant temperature. */
__code const uint8_t mixture_limit_by_coolant[39] = {
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    131, 134, 137, 140, 142, 144, 146, 148, 150, 152, 154, 156, 157,
    157, 157, 157, 157, 141, 128, 128, 128, 128, 128, 128, 128, 128,
};
