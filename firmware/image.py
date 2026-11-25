"""Convert and verify the absolute C509-L CODE image."""

import argparse
import hashlib
from pathlib import Path
import re


ROM_SIZE = 0x10000
STOCK_SHA256 = "15530e9459a23cf6a091d75547456f437136e2a7dff7c8a175aa83da2d2af8fb"
UNUSED_VECTORS = (
    0x000B, 0x0013, 0x002B, 0x0043, 0x004B, 0x0053, 0x005B,
    0x0063, 0x0083, 0x0093, 0x009B, 0x00A3, 0x00AB,
)
SYMBOLS = {
    "reset_vector": 0x0000,
    "irq_vector_0003": 0x0003,
    "irq_vector_001b": 0x001B,
    "irq_vector_0023": 0x0023,
    "irq_vector_006b": 0x006B,
    "vector_block_end": 0x00B0,
    "_set_warmup_duration": 0x280E,
    "_warmup_setup_continue": 0x2818,
    "_update_rpm_axis": 0x2BCD,
    "_rpm_update_continued": 0x2BE6,
    "_update_knock_noise_zone": 0x2C3F,
    "_knock_zone_update_continued": 0x2C6C,
    "_update_coolant_axis": 0x3396,
    "_coolant_update_continued": 0x33C2,
    "_update_load_warmup_ignition": 0x4042,
    "_load_ignition_continue": 0x406D,
    "_update_idle_ignition": 0x430D,
    "_idle_ignition_continue": 0x433A,
    "interp_bilinear_u8": 0xA59D,
    "interp_bilinear_end": 0xA5FF,
    "interp_curve_u8": 0xA611,
    "interp_curve_end": 0xA62F,
    "_minimum_u16_register_pair": 0xA33A,
    "_maximum_u16_register_pair": 0xA346,
    "_negate_u16_register_pair": 0xA381,
    "_negate_u24_register_triple": 0xA389,
    "_sign_extend_i24_register_triple": 0xA394,
    "_add_u16_register_pairs": 0xA3F9,
    "_subtract_u16_register_pairs": 0xA400,
    "_sign_extend_i8_register": 0xA408,
    "_abs_accumulator_i8": 0xA429,
    "_set_flag_2f_4_cd7c": 0xCD7C,
    "_set_flag_2f_7_d2f1": 0xD2F1,
    "_select_data_pointer_zero": 0xF090,
    "_clear_flag_21_0_f275": 0xF275,
    "_fan_off_temperature": 0x60C7,
    "_fan_on_temperature": 0x60C8,
    "_rpm_quantization_q4": 0x613C,
    "_rpm_breakpoints": 0x623C,
    "_barometric_correction_zone": 0x62AC,
    "_injection_phase_raw": 0x64D3,
    "_economy_ignition_half_degrees": 0x6AC8,
    "_egr_ignition_half_degrees": 0x6BC8,
    "_desired_idle_rpm": 0x6F56,
    "_idle_ignition_by_rpm": 0x6F7D,
    "_idle_ignition_temp_correction": 0x6F9D,
    "_cranking_small_fuel_raw": 0x710B,
    "_cranking_rpm_correction_raw": 0x7132,
    "_cranking_async_fuel_raw": 0x7152,
    "_cranking_large_fuel_raw": 0x7179,
    "_cranking_ignition_raw": 0x71A0,
    "_ignition_dwell_raw": 0x71C8,
    "_injector_dynamic_offset_raw": 0x71D8,
    "_knock_maximum_retard_raw": 0x71E8,
    "_throttle_quantization_q4": 0x7208,
    "_base_cylinder_charge_raw": 0x75EF,
    "_sensor_fault_ignition_raw": 0x7877,
    "_knock_noise_zone_flags": 0x7AC7,
    "_knock_threshold_correction_raw": 0x7E07,
    "_knock_threshold_by_rpm": 0x7F07,
    "_knock_min_cycle_interval": 0x7F27,
    "_knock_advance_recovery_period": 0x7F47,
    "_fuel_weight_by_temp_raw": 0x7F67,
    "_fuel_weight_by_throttle_raw": 0x8193,
    "_warmup_ignition_weight_raw": 0x83BA,
    "_idle_warmup_ignition": 0x84BA,
    "_load_warmup_ignition": 0x84E1,
    "_warmup_duration_by_temp": 0x8508,
    "_engine_torque_raw": 0x852F,
    "_maf_transfer_raw": 0x99B9,
    **{f"_irq_unused_{address:04x}": address for address in UNUSED_VECTORS},
}
C_SECTIONS = {
    "code_280e": 10,
    "code_2bcd": 25,
    "code_2c3f": 45,
    "code_3396": 44,
    "code_4042": 43,
    "code_430d": 45,
    "code_a33a": 24,
    "code_a381": 29,
    "code_a3f9": 25,
    "code_a429": 6,
    "code_cd7c": 3,
    "code_d2f1": 3,
    "code_f090": 4,
    "code_f275": 3,
    "const_60c7": 2,
    "const_613c": 288,
    "const_62ac": 256,
    "const_64d3": 256,
    "const_6ac8": 512,
    "const_6f56": 110,
    "const_710b": 354,
    "const_75ef": 256,
    "const_7877": 256,
    "const_7ac7": 256,
    "const_7e07": 391,
    "const_8193": 256,
    "const_83ba": 373,
    "const_852f": 256,
    "const_99b9": 256,
    **{f"code_{address:04x}": 1 for address in UNUSED_VECTORS},
}


def convert_hex(lines):
    image = bytearray(ROM_SIZE)
    present = bytearray(ROM_SIZE)
    eof = False
    for number, line in enumerate(lines, start=1):
        line = line.strip()
        if not line:
            continue
        if eof:
            raise ValueError(f"HEX line {number}: record after end of file")
        if not re.fullmatch(r":(?:[0-9a-fA-F]{2}){5,}", line):
            raise ValueError(f"HEX line {number}: invalid record")
        record = bytes.fromhex(line[1:])
        if len(record) != record[0] + 5 or sum(record) % 256:
            raise ValueError(f"HEX line {number}: invalid length or checksum")
        address = int.from_bytes(record[1:3], "big")
        kind = record[3]
        data = record[4:-1]
        if kind == 0x00:
            end = address + len(data)
            if end > ROM_SIZE:
                raise ValueError(f"HEX line {number}: outside 64 KiB CODE")
            if 1 in present[address:end]:
                first = present.index(1, address, end)
                raise ValueError(f"HEX line {number}: overlapping byte at 0x{first:04X}")
            image[address:end] = data
            present[address:end] = b"\x01" * len(data)
        elif kind == 0x01 and address == 0 and not data:
            eof = True
        elif kind in (0x02, 0x04) and address == 0 and data == b"\x00\x00":
            # Extended address records are valid only for bank zero.
            continue
        else:
            raise ValueError(f"HEX line {number}: unsupported record for 16-bit CODE")
    if not eof:
        raise ValueError("HEX: missing end-of-file record")
    if 0 in present:
        raise ValueError(f"HEX: missing byte at 0x{present.index(0):04X}")
    return image


def verify(image, reference, linker_map):
    for address, (actual, expected) in enumerate(zip(image, reference)):
        if actual != expected:
            raise ValueError(
                f"first difference at 0x{address:04X}: "
                f"built 0x{actual:02X}, expected 0x{expected:02X}"
            )
    if len(image) != len(reference):
        end = min(len(image), len(reference))
        raise ValueError(f"first difference at 0x{end:04X}: image sizes differ")
    if len(image) != ROM_SIZE or hashlib.sha256(reference).hexdigest() != STOCK_SHA256:
        raise ValueError("reference is not the stock 64 KiB J5V05L19 image")

    linked = {
        symbol: int(address, 16)
        for address, symbol in re.findall(
            r"^\s*(?:C:\s+)?([a-fA-F0-9]{4,8})\s+(\w+)\s",
            linker_map,
            flags=re.MULTILINE,
        )
    }
    for symbol, address in SYMBOLS.items():
        if linked.get(symbol) != address:
            raise ValueError(f"linker map: expected {symbol} at 0x{address:04X}")
    for section, size in C_SECTIONS.items():
        if linked.get(f"l_{section}") != size:
            raise ValueError(f"linker map: expected {size} bytes in {section}")

    flash = sum(image) & 0xFFFF
    complement = int.from_bytes(image[0xFFFA:0xFFFC], "little")
    stored_flash = int.from_bytes(image[0xFFFE:0x10000], "little")
    if flash != 0x7602 or stored_flash != flash or complement != (flash ^ 0xFFFF):
        raise ValueError("unexpected Flash checksum or complement")
    for start, end, stored_at, expected in (
        (0x6000, 0x8000, 0x5A23, 0xD3B2),
        (0x8000, 0xA000, 0xB6FD, 0x0780),
    ):
        stored = int.from_bytes(image[stored_at:stored_at + 2], "little")
        if (sum(image[start:end]) & 0xFFFF) != expected or stored != expected:
            raise ValueError(f"unexpected calibration checksum for 0x{start:04X}–0x{end - 1:04X}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subcommands = parser.add_subparsers(dest="command", required=True)
    convert = subcommands.add_parser("convert")
    convert.add_argument("hex", type=Path)
    convert.add_argument("binary", type=Path)
    check = subcommands.add_parser("verify")
    check.add_argument("binary", type=Path)
    check.add_argument("linker_map", type=Path)
    check.add_argument("reference", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "convert":
            with args.hex.open(encoding="ascii") as lines:
                image = convert_hex(lines)
            args.binary.write_bytes(image)
        else:
            verify(
                args.binary.read_bytes(),
                args.reference.read_bytes(),
                args.linker_map.read_text(encoding="ascii"),
            )
            print(f"J5V05L19: {ROM_SIZE} bytes match. SHA-256 {STOCK_SHA256}")
            print("Vectors and interpolation addresses match. Checksums: 7602, D3B2, 0780.")
            print(f"C section sizes match: {sum(C_SECTIONS.values())} bytes.")
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
