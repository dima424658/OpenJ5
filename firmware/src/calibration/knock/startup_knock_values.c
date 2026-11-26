#include <stdint.h>

volatile __code const uint8_t startup_knock_threshold = 50;
volatile __code const uint8_t startup_knock_average_noise = 50;
volatile __code const uint8_t knock_noise_filter_coefficient = 7;
volatile __code const uint8_t startup_knock_window_start = 1;
volatile __code const uint8_t startup_knock_window_end = 5;
