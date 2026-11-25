#include <stdint.h>

/* Fractional increases for the two idle transition thresholds, in Q8. */
volatile __code const uint8_t idle_transition_1_q8 = 77;
volatile __code const uint8_t idle_transition_2_q8 = 154;
