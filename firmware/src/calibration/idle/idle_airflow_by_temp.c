#include <stdint.h>

/* Coolant from -40 to 150 deg C in steps of 5 deg C; airflow in 0.2 kg/h. */
__code const uint8_t idle_airflow_by_temp[39] = {
    150, 150, 150, 150, 150, 147, 144, 141, 138, 133, 128, 123, 118,
    114, 111, 108, 100,  90,  80,  70,  60,  56,  54,  52,  50,  48,
     48,  48,  48,  48,  48,  50,  52,  54,  55,  56,  57,  58,  58,
};
