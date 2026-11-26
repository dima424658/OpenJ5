#include "flash_checksum.h"

__code const uint16_t flash_checksum_complement = (uint16_t) ~STOCK_FLASH_CHECKSUM;
__code const uint16_t flash_tail_word_fffc = 0;
