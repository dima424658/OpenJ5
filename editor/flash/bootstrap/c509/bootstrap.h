#ifndef JANVAR_BOOTSTRAP_H
#define JANVAR_BOOTSTRAP_H

#include <stdint.h>

#define MAX_FRAME_SIZE 135
#define FLASH_COPY_CHUNK 128

#ifdef __SDCC_mcs51
typedef __bit bootstrap_bit;
#define DIRECT_AT(address) __data __at(address)
#define INDIRECT __idata
#define INDIRECT_AT(address) INDIRECT __at(address)
#else
typedef uint8_t bootstrap_bit;
#define DIRECT_AT(address)
#define INDIRECT
#define INDIRECT_AT(address)
#endif

extern DIRECT_AT(0x20) uint8_t board_flags;
extern DIRECT_AT(0x60) uint16_t io_address;
extern DIRECT_AT(0x62) uint8_t io_byte;
extern DIRECT_AT(0x6b) uint8_t flash_probe_first;
extern DIRECT_AT(0x6c) uint8_t flash_probe_second;
extern INDIRECT_AT(0x70) uint8_t packet[MAX_FRAME_SIZE - 2];

uint8_t bootstrap_command(uint8_t command, uint8_t payload_length);
uint8_t bootstrap_checksum(uint8_t length, uint8_t command);

void flash_read_mode(void);
void flash_select(bootstrap_bit bank);
uint8_t flash_read_byte(void);
bootstrap_bit flash_program_byte(void);
bootstrap_bit flash_erase(void);
bootstrap_bit eeprom_read(void);
bootstrap_bit eeprom_write(void);
bootstrap_bit auxiliary_read(void);

#endif
