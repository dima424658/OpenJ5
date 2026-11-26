#include "bootstrap.h"

DIRECT_AT(0x20) uint8_t board_flags;
DIRECT_AT(0x60) uint16_t io_address;
DIRECT_AT(0x62) uint8_t io_byte;
DIRECT_AT(0x6b) uint8_t flash_probe_first;
DIRECT_AT(0x6c) uint8_t flash_probe_second;
INDIRECT_AT(0x70) uint8_t packet[MAX_FRAME_SIZE - 2];

#ifdef __SDCC_mcs51
__sfr __at(0x81) stack_pointer;
__sfr __at(0xd0) program_status;
__sbit __at(0xaf) interrupts_enabled;
static DIRECT_AT(0x64) uint8_t boot_rom_stack;
static void bootstrap_main(void);

void bootstrap_entry(void)
{
    interrupts_enabled = 0;
    boot_rom_stack = stack_pointer;
    stack_pointer = 0x3f;
    program_status = 0;
    bootstrap_main();
}
#endif

static uint8_t status_reply(bootstrap_bit error)
{
    packet[0] = error ? 0xaa : 0x55;
    return 1;
}

static void flash_select_request_bank(void)
{
    flash_select((packet[0] & 1) ^ 1);
    io_address = ((uint16_t)packet[1] << 8) | packet[2];
}

static bootstrap_bit all_flash_bytes_equal(uint8_t value)
{
    io_address = 0;
    do {
        if (flash_read_byte() != value)
            return 0;
    } while (++io_address);
    return 1;
}

static uint8_t identify_flash_layout(void)
{
    io_address = 0;
    do {
        flash_select(0);
        io_byte = flash_read_byte();
        flash_select(1);
        if (flash_read_byte() != io_byte) {
            flash_select(0);
            return status_reply(!all_flash_bytes_equal(0) && !all_flash_bytes_equal(0xff));
        }
    } while (++io_address);
    return status_reply(0);
}

static void read_flash_block(INDIRECT uint8_t *output, uint8_t count)
{
#ifdef __SDCC_mcs51
    // The Flash address wraps at the end of each 64 KiB bank.
    uint16_t address = io_address;
    do {
        *output++ = *(const volatile __xdata uint8_t *)address++;
    } while (--count);
    io_address = address;
#else
    do {
        *output++ = flash_read_byte();
        ++io_address;
    } while (--count);
#endif
}

static bootstrap_bit write_flash_block(const INDIRECT uint8_t *input, uint8_t count)
{
    do {
        io_byte = *input++;
        if (flash_program_byte())
            return 1;
        ++io_address;
    } while (--count);
    return 0;
}

static uint8_t copy_flash_bank(void)
{
    flash_select(0);
    io_address = 0;
    if (flash_read_byte() != 0xff)
        return status_reply(0);

    do {
        uint16_t base = io_address;
        flash_select(1);
        read_flash_block(packet, FLASH_COPY_CHUNK);
        io_address = base;
        flash_select(0);
        if (write_flash_block(packet, FLASH_COPY_CHUNK))
            return status_reply(1);
    } while (io_address);
    return status_reply(0);
}

uint8_t bootstrap_command(uint8_t command, uint8_t payload_length)
{
    uint8_t remaining;
    INDIRECT uint8_t *next;
    uint8_t count;
    uint8_t retries;

    switch (command) {
    case 0x10:
    case 0x18:
        return status_reply(0);
    case 0x11:
        packet[0] = flash_probe_first;
        packet[1] = flash_probe_second;
        return 2;
    case 0x12:
        flash_select(0);
        return status_reply(flash_erase());
    case 0x13:
        packet[0] = 0x55;
        packet[1] = (board_flags & 3) | ((board_flags & 4) << 1)
                                    | ((board_flags & 0x10) >> 2) | ((board_flags & 8) << 4);
        return 2;
    case 0x14:
    case 0x15:
        if (payload_length < 4)
            return 0;
        count = packet[3];
        if (!count || count > FLASH_COPY_CHUNK)
            return 0;
        if (command == 0x15 && payload_length != (uint8_t)(count + 4))
            return 0;
        flash_select_request_bank();
        if (command == 0x14) {
            read_flash_block(packet, count);
            return count;
        }
        return status_reply(write_flash_block(packet + 4, count));
    case 0x16:
    case 0x17:
        if (payload_length < 3)
            return 0;
        count = packet[2];
        if (!count || count > FLASH_COPY_CHUNK)
            return 0;
        io_address = ((uint16_t)packet[0] << 8) | packet[1];
        remaining = count;
        if (command == 0x16) {
            next = packet + 1;
            do {
                if (eeprom_read())
                    return status_reply(1);
                *next++ = io_byte;
                ++io_address;
            } while (--remaining);
            packet[0] = 0x55;
            return count + 1;
        }
        if (payload_length != (uint8_t)(count + 3))
            return 0;
        next = packet + 3;
        do {
            io_byte = *next++;
            retries = 128;
            while (eeprom_write()) {
                if (!--retries)
                    return status_reply(1);
            }
            ++io_address;
        } while (--remaining);
        return status_reply(0);
    case 0x19:
        return identify_flash_layout();
    case 0x1a:
        return copy_flash_bank();
    case 0x1b:
        if ((board_flags & 0x0e) || auxiliary_read())
            return status_reply(1);
        packet[0] = 0x55;
        return 9;
    default:
        return 0;
    }
}

uint8_t bootstrap_checksum(uint8_t length, uint8_t command)
{
    uint8_t sum = length + command;
    INDIRECT uint8_t *next = packet;
    if (length > 3) {
        uint8_t remaining = length - 3;
        do {
            sum += *next++;
        } while (--remaining);
    }
    return sum;
}

#ifdef __SDCC_mcs51
__sfr __at(0x89) timer_mode;
__sfr __at(0x8b) timer1_low;
__sfr __at(0x8d) timer1_high;
__sbit __at(0x8e) timer1_running;
__sbit __at(0x8f) timer1_overflow;
__sbit __at(0x98) uart_received;
__sbit __at(0x99) uart_transmitted;
__sfr __at(0x99) uart_data;
__sbit __at(0xab) timer1_interrupt_enabled;
__sbit __at(0xb7) flash_bank_p3_7;
__sfr __at(0xfa) port6;
DIRECT_AT(0x63) uint8_t uart_byte;


void flash_select(bootstrap_bit bank)
{
    if (board_flags & 1)
        flash_bank_p3_7 = bank;
    else
        port6 = (port6 & 0xfe) | bank;
}

uint8_t flash_read_byte(void)
{
    return *(volatile __xdata uint8_t *)io_address;
}

#define FLASH_REVERSED_BITS 0x10
#define FLASH_SWAPPED_ADDRESSES 0x20
#define FLASH_UNLOCK_REQUIRED 0x40
#define FLASH_SECTOR_ERASE 0x80
__sbit __at(0x00) alternate_board_wiring;
__sbit __at(0x04) flash_reversed_bits;
__sbit __at(0x05) flash_swapped_addresses;
__sbit __at(0x06) flash_unlock_required;
__sbit __at(0x07) flash_sector_erase;
#define FLASH(address) (*(volatile __xdata uint8_t *)(address))

static uint16_t flash_command_address(void)
{
    if (flash_swapped_addresses)
        return 0x2aaa;
    return flash_reversed_bits ? 0x32a7 : 0x5555;
}

static void flash_unlock(uint8_t command)
{
    __bit bank = alternate_board_wiring ? flash_bank_p3_7 : port6 & 1;
    flash_select(0);
    if (flash_swapped_addresses) {
        FLASH(0x2aaa) = 0xaa;
        FLASH(0x5555) = 0x55;
        FLASH(0x2aaa) = command;
    } else if (flash_reversed_bits) {
        FLASH(0x32a7) = 0x55;
        FLASH(0x4d58) = 0xaa;
        FLASH(0x32a7) = command;
    } else {
        FLASH(0x5555) = 0xaa;
        FLASH(0x2aaa) = 0x55;
        FLASH(0x5555) = command;
    }
    flash_select(bank);
}

static void flash_enter_id_mode(void)
{
    flash_unlock(flash_reversed_bits ? 0x09 : 0x90);
}

void flash_read_mode(void)
{
    if (flash_unlock_required) {
        flash_unlock(flash_reversed_bits ? 0x0f : 0xf0);
    } else {
        uint16_t address = flash_command_address();
        FLASH(address) = flash_reversed_bits ? 0x0a : 0x50;
        FLASH(address) = 0xff;
    }
}

static __bit flash_poll(uint16_t address, uint8_t expected)
{
    uint8_t data;

    if (flash_unlock_required) {
        uint8_t ready = flash_reversed_bits ? 1 : 0x80;
        uint8_t error = flash_reversed_bits ? 4 : 0x20;
        for (;;) {
            data = FLASH(address);
            if (!((data ^ expected) & ready))
                break;
            if (data & error) {
                if ((FLASH(address) ^ expected) & ready)
                    return 1;
                break;
            }
        }
    } else {
        data = FLASH(address);
        if (flash_reversed_bits && data & 1) {
            if (data & 4)
                return 1;
        } else {
            while (!(data & 0x80))
                data = FLASH(address);
            if (data & 0x20)
                return 1;
        }
    }
    return FLASH(address) != expected;
}

bootstrap_bit flash_program_byte(void)
{
    if (flash_unlock_required)
        flash_unlock(flash_reversed_bits ? 0x05 : 0xa0);
    else
        FLASH(flash_command_address()) = flash_reversed_bits ? 0x08 : 0x10;
    FLASH(io_address) = io_byte;
    return flash_poll(io_address, io_byte);
}

static __bit flash_erase_at(uint16_t address)
{
    FLASH(flash_command_address()) = flash_reversed_bits ? 0x04 : 0x20;
    FLASH(address) = flash_reversed_bits ? 0x0b : 0xd0;
    return flash_poll(address, 0xff);
}

bootstrap_bit flash_erase(void)
{
    if (flash_unlock_required) {
        flash_unlock(flash_reversed_bits ? 0x01 : 0x80);
        flash_unlock(flash_reversed_bits ? 0x08 : 0x10);
        return flash_poll(0, 0xff);
    }

    if (flash_erase_at(0))
        return 1;
    if (flash_sector_erase) {
        uint8_t sector;
        for (sector = 0x40; sector <= 0x80; sector += 0x20) {
            flash_read_mode();
            if (flash_erase_at((uint16_t)sector << 8))
                return 1;
        }
    }
    return 0;
}

static __bit flash_ids_mirrored(void)
{
    uint8_t first, second;
    flash_enter_id_mode();
    first = FLASH(0);
    if (first == 0 || first == 0xff)
        return 0;
    second = FLASH(1);
    if (second == 0 || second == 0xff)
        return 0;

    static const __code uint8_t mirror_pages[] = {0x10, 0x80, 0x90};
    uint8_t i;
    for (i = 0; i < sizeof mirror_pages; ++i) {
        uint16_t address = (uint16_t)mirror_pages[i] << 8;
        if (FLASH(address) != first || FLASH(address + 1) != second)
            return 0;
    }
    return 1;
}

static void detect_controller(void)
{
    // Keep the original probe order and the Flash flags until identification.
    board_flags &= FLASH_UNLOCK_REQUIRED | FLASH_SECTOR_ERASE;
    io_address = 0;
    if (!eeprom_read()) {
        if (!flash_ids_mirrored()) {
            board_flags |= FLASH_REVERSED_BITS;
            if (!flash_ids_mirrored())
                board_flags = (board_flags & ~FLASH_REVERSED_BITS) | 8;
        }
    } else {
        board_flags |= 1;
        if (eeprom_read()) {
            board_flags = (board_flags & ~1) | FLASH_SWAPPED_ADDRESSES | 2;
            if (eeprom_read()) {
                board_flags = (board_flags & ~2) | 4;
                if (eeprom_read())
                    board_flags = (board_flags & ~(FLASH_SWAPPED_ADDRESSES | 4)) | 8;
            }
        }
    }

    flash_select(0);
    flash_enter_id_mode();
    flash_probe_first = FLASH(0);
    flash_probe_second = FLASH(flash_swapped_addresses ? 2 : 1);
    board_flags &= ~(FLASH_UNLOCK_REQUIRED | FLASH_SECTOR_ERASE);
    if (flash_probe_first != 0x89)
        board_flags |= FLASH_UNLOCK_REQUIRED;
    static const __code uint8_t chip_ids[] = {0x51, 0xd3, 0x74, 0x23, 0x70, 0xd6, 0x9c};
    uint8_t i;
    for (i = 0; i < sizeof chip_ids; ++i) {
        if (chip_ids[i] == flash_probe_second)
            return;
    }
    board_flags |= FLASH_SECTOR_ERASE;
}

__sbit __at(0x01) eeprom_on_port9;
__sbit __at(0x02) eeprom_on_port5;
__sbit __at(0x95) eeprom_p1_5;
__sbit __at(0x97) eeprom_p1_7;
__sbit __at(0xb3) eeprom_p3_3;
__sbit __at(0xfe) eeprom_p5_6;
__sbit __at(0xff) eeprom_p5_7;
__sfr __at(0xf9) port9;
static volatile DIRECT_AT(0x65) uint8_t delay_ticks;

static void eeprom_delay(void)
{
    delay_ticks = 5;
    do {
    } while (--delay_ticks);
}

static void eeprom_data(__bit high)
{
    if (alternate_board_wiring)
        eeprom_p3_3 = high;
    else if (eeprom_on_port9) {
        if (high)
            port9 |= 0x40;
        else
            port9 &= 0xbf;
    } else if (eeprom_on_port5)
        eeprom_p5_7 = high;
    else
        eeprom_p1_5 = high;
}

static void eeprom_clock(__bit high)
{
    if (alternate_board_wiring)
        eeprom_p1_7 = high;
    else if (eeprom_on_port9) {
        if (high)
            port9 |= 0x80;
        else
            port9 &= 0x7f;
    } else if (eeprom_on_port5)
        eeprom_p5_6 = high;
    else
        eeprom_p3_3 = high;
}

static __bit eeprom_sample_data(void)
{
    if (alternate_board_wiring)
        return eeprom_p3_3;
    if (eeprom_on_port9)
        return port9 & 0x40;
    if (eeprom_on_port5)
        return eeprom_p5_7;
    return eeprom_p1_5;
}

static __bit eeprom_sample_clock(void)
{
    if (alternate_board_wiring)
        return eeprom_p1_7;
    if (eeprom_on_port9)
        return port9 & 0x80;
    if (eeprom_on_port5)
        return eeprom_p5_6;
    return eeprom_p3_3;
}

static __bit eeprom_start(void)
{
    eeprom_data(1);
    eeprom_clock(1);
    if (!eeprom_sample_data() || !eeprom_sample_clock())
        return 1;
    eeprom_delay();
    eeprom_data(0);
    eeprom_delay();
    eeprom_clock(0);
    return 0;
}

static void eeprom_stop(void)
{
    eeprom_data(0);
    eeprom_delay();
    eeprom_clock(1);
    eeprom_delay();
    eeprom_data(1);
}

static __bit eeprom_sample_bit(void)
{
    __bit high;
    eeprom_delay();
    eeprom_clock(1);
    eeprom_delay();
    high = eeprom_sample_data();
    eeprom_clock(0);
    return high;
}

static __bit eeprom_send_byte(uint8_t value)
{
    uint8_t bits = 8;
    do {
        eeprom_data(value & 0x80);
        value <<= 1;
        eeprom_delay();
        eeprom_clock(1);
        eeprom_delay();
        eeprom_clock(0);
    } while (--bits);
    eeprom_data(1);
    return eeprom_sample_bit();
}

static uint8_t eeprom_receive_byte(void)
{
    uint8_t value = 0;
    uint8_t bits = 8;
    eeprom_data(1);
    do {
        value = (value << 1) | eeprom_sample_bit();
    } while (--bits);
    return value;
}

static __bit eeprom_send_address(void)
{
    uint8_t device = 0xa0 | (((uint8_t)(io_address >> 8) << 1) & 0xfe);
    return eeprom_send_byte(device) || eeprom_send_byte((uint8_t)io_address);
}

bootstrap_bit eeprom_read(void)
{
    __bit error;
    if (eeprom_start())
        return 1;
    if (eeprom_send_address()) {
        eeprom_stop();
        return 1;
    }
    if (eeprom_start())
        return 1;

    error = eeprom_send_byte(0xa1 | ((uint8_t)(io_address >> 8) << 1));
    if (!error)
        io_byte = eeprom_receive_byte();
    eeprom_stop();
    return error;
}

bootstrap_bit eeprom_write(void)
{
    uint8_t expected = io_byte;
    __bit error;
    if (eeprom_start())
        return 1;

    error = eeprom_send_address() || eeprom_send_byte(expected);
    eeprom_stop();
    if (error)
        return 1;

    eeprom_delay();
    while (eeprom_read()) {
    }
    error = io_byte != expected;
    io_byte = expected;
    return error;
}

__sfr __at(0xb9) auxiliary_port_mode;
__sfr __at(0xf8) port5;

/* Five machine cycles per iteration in the SDCC build. The reset and
 * bit slots are based on the original C509-L timing at 16 MHz. */
static void delay_cycles(uint8_t count)
{
    delay_ticks = count;
    do {
    } while (--delay_ticks);
}

static inline void auxiliary_low(void)
{
    if (alternate_board_wiring)
        port6 &= 0xdf;
    else
        port5 &= 0x7f;
}

static inline void auxiliary_release(void)
{
    if (alternate_board_wiring)
        port6 |= 0x20;
    else
        port5 |= 0x80;
}

static inline __bit auxiliary_high(void)
{
    if (alternate_board_wiring)
        return port6 & 0x20;
    return port5 & 0x80;
}

static __bit auxiliary_reset(void)
{
    uint8_t remaining;
    auxiliary_port_mode |= 0x80;
    auxiliary_low();
    delay_cycles(0);
    auxiliary_release();

    remaining = 22;
    while (auxiliary_high()) {
        if (!--remaining)
            return 1;
    }
    remaining = 91;
    while (!auxiliary_high()) {
        if (!--remaining)
            return 1;
    }
    delay_cycles(126);
    return 0;
}

static void auxiliary_send_byte(uint8_t byte)
{
    uint8_t bits = 8;
    do {
        auxiliary_low();
        if (byte & 1)
            auxiliary_release();
        delay_cycles(31);
        auxiliary_release();
        byte >>= 1;
    } while (--bits);
}

static __bit auxiliary_read_bit(void)
{
    __bit high;
    auxiliary_port_mode |= 0x80;
    auxiliary_low();
    auxiliary_release();
    high = auxiliary_high();
    delay_cycles(31);
    return high;
}

bootstrap_bit auxiliary_read(void)
{
    uint8_t i, bits, data;
    uint8_t crc = 0;

    if (auxiliary_reset())
        return 1;
    auxiliary_send_byte(0x0f);
    for (i = 1; i <= 8; ++i) {
        bits = 8;
        data = 0;
        do {
            __bit high = auxiliary_read_bit();
            __bit mix = (crc & 1) != high;
            data = (data >> 1) | (high ? 0x80 : 0);
            crc = (crc >> 1) ^ (mix ? 0x8c : 0);
        } while (--bits);
        packet[i] = data;
    }
    return crc != 0;
}

static __bit uart_read(void)
{
    timer1_interrupt_enabled = 0;
    timer1_running = 0;
    timer1_overflow = 0;
    timer_mode &= 0x0f;
    timer_mode |= 0x10;
    timer1_high = 0;
    timer1_low = 0;
    timer1_overflow = 0;
    timer1_running = 1;

    while (!timer1_overflow) {
        if (uart_received) {
            uart_byte = uart_data;
            uart_received = 0;
            break;
        }
    }
    return timer1_overflow;
}

static void uart_write(void)
{
    uart_transmitted = 0;
    uart_data = uart_byte;
    while (!uart_transmitted) {
    }
}

static DIRECT_AT(0x6e) uint8_t frame_length;
static DIRECT_AT(0x6f) uint8_t frame_command;

static __bit receive_request(void)
{
    uint8_t remaining;
    do {
        __idata uint8_t *next = packet;
        while (uart_read()) {
        }
        frame_length = uart_byte;
        if (frame_length < 4 || frame_length > MAX_FRAME_SIZE)
            return 0;
        if (uart_read())
            continue;
        frame_command = uart_byte;
        remaining = frame_length - 2;
        do {
            if (uart_read())
                break;
            *next++ = uart_byte;
        } while (--remaining);
        if (!remaining)
            return 1;
    } while (1);
}

__sfr __at(0x8a) timer0_low;
__sfr __at(0x8c) timer0_high;
__sfr __at(0xb2) memory_mode;
__sfr __at(0xe0) accumulator;
__sfr __at(0xf0) b_register;

static void start_application(void)
{
    __idata uint8_t *reset_address = (__idata uint8_t *)(uint8_t)(boot_rom_stack + 1);

    delay_cycles(102);
    timer0_low = 0;
    timer0_high = 0;
    reset_address[0] = 0;
    reset_address[1] = 0;
    memory_mode &= 0xfe;
    memory_mode |= 0x84;
    memory_mode |= 0x40;
    // RET consumes the reset address and leaves the original stack pointer.
    stack_pointer = boot_rom_stack + 2;
    b_register = 0;
    accumulator = 0;
}

static void bootstrap_main(void)
{
    detect_controller();
    for (;;) {
        __idata uint8_t *next = packet;
        uint8_t remaining;
        uint8_t reply = 0;

        flash_read_mode();
        flash_select(1);
        uart_received = 0;
        if (receive_request()) {
            uint8_t length = frame_length - 3;
            if (packet[length] == bootstrap_checksum(frame_length, frame_command))
                reply = bootstrap_command(frame_command, length);
        }
        if (!reply) {
            frame_command = 0x1f;
            packet[0] = 0xff;
            reply = 1;
        }
        frame_command |= 0x90;
        frame_length = reply + 3;
        packet[reply] = bootstrap_checksum(frame_length, frame_command);
        uart_byte = frame_length;
        uart_write();
        uart_byte = frame_command;
        uart_write();
        remaining = reply + 1;
        do {
            uart_byte = *next++;
            uart_write();
        } while (--remaining);

        if (frame_command == 0x90)
            start_application();
    }
}
#endif
