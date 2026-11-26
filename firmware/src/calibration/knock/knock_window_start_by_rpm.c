#include <stdint.h>

/* The raw measurement-window phases use six degrees per count. */
__code const uint8_t knock_window_start_by_rpm[32] = {
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
};

__code const uint8_t knock_window_end_by_rpm[32] = {
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 8, 8, 8, 8, 9, 9, 9,
    9, 9, 10, 10, 10, 10, 10, 10,
};
