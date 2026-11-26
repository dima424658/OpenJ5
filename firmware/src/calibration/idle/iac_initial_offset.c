#include <stdint.h>

volatile __code const int8_t iac_initial_offset = -12;

/* Position in steps, gain in Q7 and airflow in units of 0.2 kg/h. */
volatile __code const uint8_t iac_max_position = 120;
volatile __code const uint8_t iac_airflow_gain_q7 = 122;
volatile __code const uint8_t iac_min_airflow = 30;
volatile __code const uint8_t iac_max_airflow = 150;
