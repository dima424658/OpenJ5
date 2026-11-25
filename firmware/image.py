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
    "_clear_fuel_history_f839": 0x2041,
    "_set_warmup_duration": 0x280E,
    "_warmup_setup_continue": 0x2818,
    "_update_rpm_axis": 0x2BCD,
    "_rpm_update_continued": 0x2BE6,
    "_update_knock_noise_zone": 0x2C3F,
    "_knock_zone_update_continued": 0x2C6C,
    "_update_coolant_axis": 0x3396,
    "_coolant_update_continued": 0x33C2,
    "_update_idle_or_economy_mixture": 0x3F74,
    "_update_base_ignition": 0x3F95,
    "_update_injection_phase": 0x3FAD,
    "_update_knock_threshold_scale": 0x3FBB,
    "_update_knock_rpm_parameters": 0x3FEF,
    "_update_fuel_weighting": 0x400D,
    "_update_load_warmup_ignition": 0x4042,
    "_update_engine_torque": 0x406D,
    "_load_ignition_tail_continue": 0x4077,
    "_update_sensor_fault_targets": 0x4081,
    "_finish_sensor_fault": 0x409A,
    "_update_idle_speed_target": 0x428D,
    "_update_idle_transition_rpm": 0x42B5,
    "_apply_moving_idle_rpm_offset": 0x42E9,
    "_update_idle_ignition": 0x430D,
    "_update_idle_airflow": 0x433A,
    "_select_idle_or_fuel_cut": 0x4383,
    "_update_idle_or_cut_from_coast": 0x43D8,
    "_try_enter_regulated_idle": 0x4405,
    "_leave_idle_for_load": 0x4427,
    "_run_airflow_control": 0x4450,
    "_enter_regulated_idle": 0x4453,
    "_enter_poststart_idle": 0x4490,
    "_keep_idle_feedback": 0x44C0,
    "_arm_coast_fuel_cut": 0x44DB,
    "_check_coast_fuel_cut_delay": 0x44E3,
    "_begin_coast_fuel_cut": 0x4546,
    "_update_coast_fuel_cut": 0x454E,
    "_check_fuel_cut_recovery": 0x4598,
    "_apply_deceleration_fuel_cut": 0x45F5,
    "_apply_idle_airflow": 0x4613,
    "_continue_after_idle_airflow": 0x461F,
    "_finish_ignition_corrections": 0x4912,
    "_update_cooling_fan": 0x4B1A,
    "_apply_fan_delay": 0x4B45,
    "_publish_fan_request": 0x4B6F,
    "_cooling_fan_continue": 0x4B81,
    "_update_idle_ignition_feedback": 0x9DC8,
    "_calculate_idle_rpm_error": 0x9E2F,
    "_apply_idle_ignition_correction": 0x9E62,
    "_limit_and_store_idle_ignition": 0x9EA3,
    "_prepare_idle_airflow_feedback": 0x9ED3,
    "_calculate_idle_rpm_delta": 0x9EDA,
    "_suppress_cold_iac_delta": 0x9F1D,
    "_apply_idle_airflow_feedback": 0x9F32,
    "_scale_idle_iac_feedback": 0x9F65,
    "_apply_idle_iac_position_delta": 0x9FA1,
    "_update_idle_iac_feedback": 0xA020,
    "_prepare_iac_adaptation_window": 0xA027,
    "_accumulate_iac_adapt_sample": 0xA054,
    "_evaluate_iac_adaptation_window": 0xA0B3,
    "_calculate_iac_error_spread": 0xA0D7,
    "_update_iac_rpm_trim": 0xA112,
    "_continue_iac_adaptation": 0xA153,
    "_update_iac_adaptation_ready": 0xA1DE,
    "_update_idle_transition_hold": 0xA214,
    "_finish_iac_adaptation_window": 0xA22A,
    "_interp_curve_q4": 0xA575,
    "_interp_bilinear_u8": 0xA59D,
    "_blend_u8_q8": 0xA5FF,
    "_interp_curve_u8": 0xA611,
    "_interp_bilinear_xdata": 0xA62F,
    "_clear_iac_history": 0xA22E,
    "_clamp_stock_u8": 0xA314,
    "_limit_stock_i8_by_sign": 0xA325,
    "_minimum_u16_register_pair": 0xA33A,
    "_maximum_u16_register_pair": 0xA346,
    "_multiply_stock_u16_u8": 0xA352,
    "_multiply_stock_i16_u8": 0xA364,
    "_negate_u16_register_pair": 0xA381,
    "_negate_u24_register_triple": 0xA389,
    "_sign_extend_i24_register_triple": 0xA394,
    "_shift_left_saturating_1": 0xA39E,
    "_saturate_stock_word": 0xA3A7,
    "_shift_left_saturating_2": 0xA3AC,
    "_shift_left_saturating_3": 0xA3BA,
    "_shift_left_saturating_4": 0xA3C8,
    "_shift_right_u16_1": 0xA3D9,
    "_shift_right_u16_2": 0xA3E1,
    "_shift_right_u16_3": 0xA3ED,
    "_add_u16_register_pairs": 0xA3F9,
    "_subtract_u16_register_pairs": 0xA400,
    "_sign_extend_i8_register": 0xA408,
    "_saturate_stock_i16_to_i8": 0xA412,
    "_abs_accumulator_i8": 0xA429,
    "_abs_stock_i16": 0xA42F,
    "_apply_iac_airflow_target": 0xA43E,
    "_convert_iac_airflow_to_position": 0xA4A5,
    "_apply_poststart_iac_airflow": 0xA4DC,
    "_reset_poststart_iac_offset": 0xA524,
    "_convert_poststart_iac_airflow": 0xA52D,
    "_update_fuel_cut_mixture": 0xA84D,
    "_set_flag_2f_4_cd7c": 0xCD7C,
    "_set_flag_2f_7_d2f1": 0xD2F1,
    "_select_data_pointer_zero": 0xF090,
    "_clear_flag_21_0_f275": 0xF275,
    "_idle_transition_1_ac_q8": 0x5F57,
    "_cold_start_iac_max_position": 0x5F62,
    "_cold_start_coolant_threshold": 0x5F63,
    "_poststart_fuel_decay_gain": 0x5F6D,
    "_iac_open_throttle_offset": 0x5F77,
    "_dynamic_ignition_temp_limit": 0x5F8D,
    "_iac_min_position": 0x5FA3,
    "_iac_ac_min_position": 0x5FAF,
    "_fuel_cut_speed_threshold": 0x607B,
    "_iac_initial_offset": 0x6081,
    "_iac_max_position": 0x6082,
    "_iac_airflow_gain_q7": 0x6083,
    "_iac_min_airflow": 0x6084,
    "_iac_max_airflow": 0x6085,
    "_idle_transition_1_q8": 0x6086,
    "_idle_transition_2_q8": 0x6087,
    "_fuel_cut_coolant_threshold": 0x608A,
    "_closed_throttle_position": 0x608B,
    "_fuel_cut_delay_ticks": 0x608D,
    "_fuel_cut_ignition_delay_ticks": 0x608E,
    "_idle_rpm_ramp_period_above": 0x608F,
    "_idle_rpm_ramp_period_below": 0x6090,
    "_iac_feedback_proportional_gain": 0x6091,
    "_iac_feedback_integral_gain": 0x6092,
    "_idle_ignition_gain_nonnegative": 0x6093,
    "_idle_ignition_gain_negative": 0x6094,
    "_idle_ignition_deadband": 0x6095,
    "_idle_error_spread_low": 0x6098,
    "_idle_error_spread_high": 0x6099,
    "_idle_rpm_trim_min": 0x609A,
    "_idle_rpm_trim_max": 0x609B,
    "_sensor_fault_mixture_raw": 0x60AD,
    "_fan_off_temperature": 0x60C7,
    "_fan_on_temperature": 0x60C8,
    "_moving_idle_rpm_offset": 0x60E5,
    "_fuel_resume_rpm_threshold": 0x60E9,
    "_fuel_cut_rpm_threshold": 0x60EA,
    "_iac_integral_error_limit": 0x60EB,
    "_fan_iac_offset": 0x60ED,
    "_fan_start_delay_ticks": 0x60EE,
    "_iac_adaptation_delay_warm": 0x60EF,
    "_iac_adaptation_delay_cold": 0x60F5,
    "_iac_cold_max_position": 0x60F6,
    "_idle_iac_cold_temp_limit": 0x60F7,
    "_iac_adaptation_min_offset": 0x60F8,
    "_iac_adaptation_max_offset": 0x60F9,
    "_iac_adaptation_period": 0x60FA,
    "_rpm_quantization_q4": 0x613C,
    "_rpm_breakpoints": 0x623C,
    "_barometric_correction_zone": 0x62AC,
    "_injection_phase_raw": 0x64D3,
    "_economy_mixture_raw": 0x6613,
    "_catalyst_mixture_raw": 0x6713,
    "_initial_injection_correction": 0x6813,
    "_minimum_ignition_offset_raw": 0x6913,
    "_maximum_ignition_offset_raw": 0x6933,
    "_power_mixture_raw": 0x6953,
    "_base_mixture_by_coolant": 0x6A53,
    "_base_mixture_temp_correction": 0x6A7A,
    "_economy_ignition_half_degrees": 0x6AC8,
    "_egr_ignition_half_degrees": 0x6BC8,
    "_desired_idle_rpm": 0x6F56,
    "_idle_ignition_by_rpm": 0x6F7D,
    "_idle_ignition_temp_correction": 0x6F9D,
    "_fuel_cut_ignition_half_degrees": 0x6FC4,
    "_idle_airflow_by_temp": 0x6FE4,
    "_iac_feedback_stiffness": 0x700B,
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
    "_idle_ac_airflow_by_temp": 0x866F,
    "_poststart_fuel_decay_by_temp": 0x8796,
    "_idle_mixture_by_coolant": 0x87E4,
    "_closed_throttle_by_rpm": 0x8AC0,
    "_open_throttle_by_rpm": 0x8AE0,
    "_poststart_iac_upper_offset_8c00": 0x8C00,
    "_poststart_iac_lower_offset_8c27": 0x8C27,
    "_poststart_iac_period_8c4e": 0x8C4E,
    "_idle_airflow_rpm_gain_q8": 0x98F2,
    "_maf_transfer_raw": 0x99B9,
    **{f"_irq_unused_{address:04x}": address for address in UNUSED_VECTORS},
}
C_SECTIONS = {
    "code_2041": 12,
    "code_280e": 10,
    "code_2bcd": 25,
    "code_2c3f": 45,
    "code_3396": 44,
    "code_3f74": 33,
    "code_3f95": 24,
    "code_3fad": 14,
    "code_3fbb": 52,
    "code_3fef": 30,
    "code_400d": 53,
    "code_4042": 43,
    "code_406d": 10,
    "code_4081": 25,
    "code_428d": 128,
    "code_430d": 45,
    "code_433a": 73,
    "code_4383": 164,
    "code_4427": 44,
    "code_4453": 136,
    "code_44db": 115,
    "code_454e": 74,
    "code_4598": 135,
    "code_4b1a": 103,
    "code_9dc8": 154,
    "code_9e62": 113,
    "code_9ed3": 95,
    "code_9f32": 238,
    "code_a020": 147,
    "code_a0b3": 160,
    "code_a153": 139,
    "code_a1de": 80,
    "code_a22e": 12,
    "code_a314": 38,
    "code_a33a": 24,
    "code_a352": 47,
    "code_a381": 29,
    "code_a39e": 59,
    "code_a3d9": 32,
    "code_a3f9": 25,
    "code_a412": 23,
    "code_a429": 6,
    "code_a42f": 15,
    "code_a43e": 158,
    "code_a4dc": 153,
    "code_a575": 40,
    "code_a59d": 98,
    "code_a5ff": 18,
    "code_a611": 30,
    "code_a62f": 108,
    "code_cd7c": 3,
    "code_d2f1": 3,
    "code_f090": 4,
    "code_f275": 3,
    "const_5f57": 1,
    "const_5f62": 2,
    "const_5f6d": 1,
    "const_5f77": 1,
    "const_5f8d": 1,
    "const_5fa3": 1,
    "const_5faf": 1,
    "const_607b": 1,
    "const_6081": 5,
    "const_6086": 2,
    "const_608a": 1,
    "const_608b": 1,
    "const_608d": 2,
    "const_608f": 2,
    "const_6091": 2,
    "const_6093": 3,
    "const_6098": 4,
    "const_60ad": 1,
    "const_60c7": 2,
    "const_60e5": 1,
    "const_60e9": 1,
    "const_60ea": 1,
    "const_60eb": 1,
    "const_60ed": 2,
    "const_60ef": 1,
    "const_60f5": 3,
    "const_60f8": 3,
    "const_613c": 288,
    "const_62ac": 256,
    "const_64d3": 256,
    "const_6613": 1166,
    "const_6ac8": 512,
    "const_6f56": 110,
    "const_6fc4": 32,
    "const_6fe4": 39,
    "const_700b": 256,
    "const_710b": 354,
    "const_75ef": 256,
    "const_7877": 256,
    "const_7ac7": 256,
    "const_7e07": 391,
    "const_8193": 256,
    "const_83ba": 373,
    "const_852f": 256,
    "const_866f": 39,
    "const_8796": 39,
    "const_87e4": 39,
    "const_8ac0": 64,
    "const_8c00": 117,
    "const_98f2": 32,
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
