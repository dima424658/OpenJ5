#include <stdint.h>

/* Airflow gain in Q8; two samples per RPM bin. */
__code const uint8_t idle_airflow_rpm_gain_q8[32] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 238, 219, 189, 174, 150, 135, 122, 111, 101,  92,  84,  77,  70,  67,  64,
};
