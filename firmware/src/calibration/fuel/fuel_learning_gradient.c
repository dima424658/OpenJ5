#include <stdint.h>

/* Maximum raw change of the allowed learned correction per cell of distance. */
volatile __code const uint8_t fuel_learning_gradient = 5;
