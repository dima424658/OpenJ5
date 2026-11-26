#include <stdint.h>

/* Exhaust-pipe temperature filter gain in Q8, indexed by airflow. */
__code const uint8_t exhaust_pipe_filter_q8[32] = {
    2, 2, 2, 2, 3, 4, 4, 5,
    5, 6, 6, 6, 6, 7, 7, 7,
    8, 8, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9,
};
