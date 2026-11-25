use super::*;

const SAMPLE: &[u8; IMAGE_SIZE] = include_bytes!("../../../bins/J5v05l19.bin");
const I02: &[u8] = include_bytes!("../../../bins/J5v13i02.bin");
const VENDOR: &[u8] = include_bytes!("../../../bins/J5v05l19a.bin");

fn readback(image: &mut Image) -> Vec<u8> {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("output.bin");
    image.save(&path, SaveMode::Create).unwrap();
    assert!(
        open(&path)
            .unwrap()
            .checksums()
            .iter()
            .all(|sum| sum.valid())
    );
    std::fs::read(path).unwrap()
}

fn changed(before: &[u8], after: &[u8]) -> Vec<usize> {
    before
        .iter()
        .zip(after)
        .enumerate()
        .filter_map(|(index, (a, b))| (a != b).then_some(index))
        .collect()
}

fn calibration_changes(before: &[u8], after: &[u8]) -> Vec<usize> {
    changed(before, after).into_iter()
        .filter(|offset| !matches!(offset, 0x5A23..=0x5A24 | 0xB6FD..=0xB6FE | 0xFFFA..=0xFFFB | 0xFFFE..=0xFFFF))
        .collect()
}

#[test]
fn scalar_temperatures_match_ctp_edits() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    assert_eq!(image.value(Parameter::FanOn, 0).unwrap(), 101.0);
    assert_eq!(image.value(Parameter::FanOff, 0).unwrap(), 98.0);
    image.set(Parameter::FanOn, 0, 42.0).unwrap();
    image.set(Parameter::FanOff, 0, 150.0).unwrap();
    let bin = readback(&mut image);
    assert_eq!(calibration_changes(SAMPLE, &bin), [0x60C7, 0x60C8]);
    assert_eq!(bin[0x60C7], 0xBE);
    assert_eq!(bin[0x60C8], 0x52);
    let decoded = Image::new(bin).unwrap();
    assert_eq!(decoded.value(Parameter::FanOn, 0).unwrap(), 42.0);
    assert_eq!(decoded.value(Parameter::FanOff, 0).unwrap(), 150.0);
}

#[test]
fn two_dimensional_curve_round_trips_only_selected_node() {
    for stock in [SAMPLE.as_slice(), I02] {
        let mut image = Image::new(stock.to_vec()).unwrap();
        assert_eq!(image.value(Parameter::Idle, 0).unwrap(), 1600.0);
        assert_eq!(
            image.value(Parameter::Idle, 24).unwrap(),
            if stock == I02 { 860.0 } else { 820.0 }
        );
        image.set(Parameter::Idle, 24, 910.0).unwrap();
        assert!(image.undo());
        assert!(image.differences().next().is_none());
        assert!(image.redo());
        let bin = readback(&mut image);
        assert_eq!(calibration_changes(stock, &bin), [0x6F56 + 24]);
        assert_eq!(bin[0x6F56 + 24], 91);
        assert_eq!(
            Image::new(bin).unwrap().value(Parameter::Idle, 24).unwrap(),
            910.0
        );
    }
}

#[test]
fn three_dimensional_table_round_trips_signed_ctp_sample() {
    let parameter = Parameter::Ignition;
    let cell = TableCell::new(1, 1, parameter).unwrap();
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    let index = cell.index();
    assert_eq!(index, 17);
    assert_eq!(image.rpm(cell.column()), Some(720));
    assert_eq!(image.rpm(15), Some(7500));
    assert_eq!(image.load_axis(cell.row()), Some(72));
    assert_eq!(image.value(parameter, index).unwrap(), 21.0);
    image.set(parameter, index, -25.5).unwrap();
    let bin = readback(&mut image);
    assert_eq!(calibration_changes(SAMPLE, &bin), [0x6AD9]);
    assert_eq!(bin[0x6AD9], 0xCD);
    assert_eq!(
        Image::new(bin).unwrap().value(parameter, index).unwrap(),
        -25.5
    );
}

#[test]
fn table_coordinates_preserve_column_order_and_edges() {
    let param = Parameter::Ignition;
    assert_eq!(TableCell::new(1, 0, param).unwrap().index(), 1);
    assert_eq!(TableCell::new(0, 1, param).unwrap().index(), 16);
    assert_eq!(TableCell::new(15, 15, param).unwrap().index(), 255);
    for index in 0..256 {
        assert_eq!(TableCell::from_index(index, param).unwrap().index(), index);
    }
    assert_eq!(TableCell::from_index(256, param), None);
    assert_eq!(TableCell::new(16, 2, param), None);
    assert_eq!(TableCell::new(0, 0, Parameter::Idle), None);
}

#[test]
fn encodings_round_trip_every_possible_byte() {
    for parameter in Parameter::ALL.into_iter().filter(|param| param.writable()) {
        for raw in 0..=255 {
            assert_eq!(parameter.encode(parameter.decode(raw)).unwrap(), raw);
        }
    }
}

#[test]
fn invalid_values_and_unverified_data_leave_image_untouched() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    for (param, index, input) in [
        (Parameter::FanOn, 0, f64::NAN),
        (Parameter::FanOn, 0, f64::INFINITY),
        (Parameter::Idle, 39, 950.0),
        (Parameter::Ignition, 256, 0.0),
        (Parameter::AirCharge, 1, 127.0),
    ] {
        assert!(image.set(param, index, input).is_err());
    }
    assert!(!image.can_undo());
    assert_eq!(image.to_bin(), SAMPLE);
}

#[test]
fn detects_known_calibration_protection_variants() {
    for (bin, mode) in [
        (SAMPLE.as_slice(), RomMode::Stock),
        (VENDOR, RomMode::VendorDisabled),
        (I02, RomMode::Unsupported),
    ] {
        assert_eq!(Image::new(bin.to_vec()).unwrap().rom_mode(), mode);
    }
}

#[test]
fn disable_and_restore_stock_are_inverse() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    image.set_rom_control(false).unwrap();
    assert_eq!(image.rom_mode(), RomMode::CtpDisabled);
    assert_eq!(&image.bytes()[0x6000..0xA000], &SAMPLE[0x6000..0xA000]);
    assert!(image.undo());
    assert_eq!(image.to_bin(), SAMPLE);
    assert!(image.redo());
    assert_eq!(image.rom_mode(), RomMode::CtpDisabled);
    let saved = readback(&mut image);
    assert_eq!(Image::new(saved).unwrap().rom_mode(), RomMode::CtpDisabled);
    image.set_rom_control(true).unwrap();
    assert_eq!(image.rom_mode(), RomMode::Stock);
    assert_eq!(readback(&mut image), SAMPLE);
}

#[test]
fn restoration_preserves_user_calibrations_and_vendor_metadata() {
    let mut disabled = Image::new(SAMPLE.to_vec()).unwrap();
    disabled.set_rom_control(false).unwrap();
    for mut image in [disabled, Image::new(VENDOR.to_vec()).unwrap()] {
        image.set(Parameter::FanOn, 0, 114.0).unwrap();
        let calibrated = image.to_bin();
        image.set_rom_control(true).unwrap();
        let restored = readback(&mut image);
        assert_eq!(restored[..0x5A23], SAMPLE[..0x5A23]);
        for range in [0x6000..0x8000, 0xF294..0xF295, 0xFFFC..0xFFFE] {
            assert_eq!(restored[range.clone()], calibrated[range]);
        }
        assert_eq!(image.rom_mode(), RomMode::Stock);
    }
}

#[test]
fn protection_rejects_partial_unknown_and_unsupported_code() {
    let mut partial = SAMPLE.to_vec();
    partial[0x2530] = 0;
    partial[0x2531] = 0;
    let mut changed_context = SAMPLE.to_vec();
    changed_context[0x4D18] ^= 1;
    for (bin, mode) in [
        (partial, RomMode::Unknown),
        (changed_context, RomMode::Unknown),
        (I02.to_vec(), RomMode::Unsupported),
    ] {
        let mut image = Image::new(bin.clone()).unwrap();
        assert_eq!(image.rom_mode(), mode);
        assert!(image.set_rom_control(false).is_err());
        assert!(image.set_rom_control(true).is_err());
        assert!(!image.can_undo());
        assert_eq!(image.data, bin);
    }
}

#[test]
fn full_and_regional_checksums_match_independent_calculation() {
    for (stock, expected) in [(SAMPLE.as_slice(), 0x7602), (I02, 0xB4CE)] {
        let mut image = Image::new(stock.to_vec()).unwrap();
        assert_eq!(image.checksums()[0].stored, expected);
        assert_eq!(image.checksums()[0].calculated, expected);
        image.set(Parameter::FanOn, 0, -40.0).unwrap();
        let bin = readback(&mut image);
        let checksum =
            |bytes: &[u8]| bytes.iter().map(|&value| u32::from(value)).sum::<u32>() as u16;
        assert_eq!(word(&bin, 0xFFFE), checksum(&bin));
        assert_eq!(word(&bin, 0xFFFA), !checksum(&bin));
        assert_eq!(&bin[0xFFFC..0xFFFE], &stock[0xFFFC..0xFFFE]);
        if image.profile() == Profile::L19 {
            assert_eq!(word(&bin, 0x5A23), checksum(&bin[0x6000..0x8000]));
            assert_eq!(word(&bin, 0xB6FD), checksum(&bin[0x8000..0xA000]));
        } else {
            assert_eq!(image.checksums().len(), 2);
            assert_eq!(word(&bin, 0x5A23), word(stock, 0x5A23));
        }
    }
}

#[test]
fn repairing_stale_regional_checksum_updates_both_checksum_levels() {
    let mut edited = SAMPLE.to_vec();
    edited[0x60C8] -= 1;
    let total = flash_sum(&edited);
    put_word(&mut edited, FLASH_COMPLEMENT, !total);
    put_word(&mut edited, FLASH_SUM, total);
    let mut image = Image::new(edited.clone()).unwrap();
    assert!(image.checksums()[0].valid());
    assert!(!image.checksums()[2].valid());
    let saved = readback(&mut image);
    assert!(calibration_changes(&edited, &saved).is_empty());
    assert_eq!(word(&saved, 0x5A23), 0xD3B1);
}

#[test]
fn saving_failed_copy_preserves_destination_and_edit_history() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("original.bin");
    std::fs::write(&path, SAMPLE).unwrap();
    image.set(Parameter::FanOn, 0, 42.0).unwrap();
    assert!(image.save(&path, SaveMode::Create).is_err());
    assert_eq!(std::fs::read(&path).unwrap(), SAMPLE);
    assert_eq!(
        image.differences().collect::<Vec<_>>(),
        [(Parameter::FanOn, 0)]
    );
    assert!(image.can_undo());
    image.save(&path, SaveMode::Replace).unwrap();
    assert!(image.differences().next().is_none());
    assert!(image.undo());
    assert_eq!(
        image.differences().collect::<Vec<_>>(),
        [(Parameter::FanOn, 0)]
    );
}

#[test]
fn rejects_invalid_length_and_incompatible_bin_layout() {
    assert!(Image::new(vec![0; IMAGE_SIZE - 1]).is_err());
    assert!(Image::new(vec![0; IMAGE_SIZE]).is_err());
    let mut incompatible = SAMPLE.to_vec();
    incompatible[0x5317] = 0;
    assert!(Image::new(incompatible).is_err());
}

#[test]
fn curve_batch_quantizes_and_undoes_as_one_edit() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    let curve = Parameter::Idle;
    image.apply_batch(curve, Batch::Percent, 5.0).unwrap();
    assert_eq!(image.value(curve, 0).unwrap(), 1680.0);
    assert_eq!(image.value(curve, 24).unwrap(), 860.0);
    assert_eq!(image.differences().count(), 39);
    assert!(image.undo());
    assert!(!image.can_undo());
    assert_eq!(image.to_bin(), SAMPLE);
    assert!(image.redo());
    assert_eq!(readback(&mut image)[0x6F56], 168);
}

#[test]
fn surface_batch_quantizes_and_clamps_all_nodes_atomically() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    let table = Parameter::Ignition;
    assert!(image.apply_batch(table, Batch::Set, f64::NAN).is_err());
    assert!(!image.can_undo());
    image.apply_batch(table, Batch::Set, -25.3).unwrap();
    assert_eq!(image.to_bin()[0x6AC8..0x6BC8], [0xCD; 256]);
    image.apply_batch(table, Batch::Percent, f64::MAX).unwrap();
    assert_eq!(image.to_bin()[0x6AC8..0x6BC8], [0x80; 256]);
    assert!(image.undo());
    assert_eq!(image.to_bin()[0x6AC8..0x6BC8], [0xCD; 256]);
    assert!(image.undo());
    assert_eq!(image.to_bin(), SAMPLE);
}

#[test]
fn out_of_bounds_reads_and_table_coordinates_are_rejected() {
    let image = Image::new(SAMPLE.to_vec()).unwrap();
    assert_eq!(image.value(Parameter::FanOn, 1), None);
    assert_eq!(image.reading(Parameter::Idle, 39), None);
    assert_eq!(image.value(Parameter::Ignition, usize::MAX), None);
    assert_eq!(TableCell::new(0, usize::MAX, Parameter::Ignition), None);
    assert_eq!(image.rpm(16), None);
    assert_eq!(image.load_axis(usize::MAX), None);
    assert_eq!(idle_temperature(0), Some(-40));
    assert_eq!(idle_temperature(24), Some(80));
    assert_eq!(idle_temperature(38), Some(150));
    assert_eq!(idle_temperature(39), None);
}

#[test]
fn passport_text_round_trips_in_known_slots_for_both_profiles() {
    for (stock, date, software) in [
        (SAMPLE.as_slice(), "08-05-2003", "J5V05L19"),
        (I02, "27-03-2000", "J5V13I02"),
    ] {
        let mut image = Image::new(stock.to_vec()).unwrap();
        assert_eq!(image.passport(PassportField::ProgrammingDate), Some(date));
        assert_eq!(
            image.passport(PassportField::SoftwareIdentifier),
            Some(software)
        );
        image
            .set_passport(PassportField::EngineType, "SAMARA")
            .unwrap();
        assert_eq!(image.passport(PassportField::EngineType), Some("SAMARA"));
        assert!(image.undo());
        assert!(!image.has_changes());
        assert!(image.redo());
        let (address, width) = PassportField::EngineType.slot(image.profile());
        let output = readback(&mut image);
        assert_eq!(&output[address..address + width + 1], b"SAMARA         \0");
        assert!(
            calibration_changes(stock, &output)
                .into_iter()
                .all(|byte| (address..address + width).contains(&byte))
        );
        assert_eq!(&output[0x6020..0x6028], software.as_bytes());
        let reopened = Image::new(output).unwrap();
        assert_eq!(reopened.passport(PassportField::EngineType), Some("SAMARA"));
    }
}

#[test]
fn supplier_passport_updates_both_copies_and_the_kwp_checksum() {
    use PassportField::{SupplierHardwareNumber, SupplierSoftwareNumber};

    for (stock, software, hardware_reply) in [
        (SAMPLE.as_slice(), "1411010-41", 0xC8D8),
        (I02, "1411010-71", 0xC53B),
    ] {
        let mut image = Image::new(stock.to_vec()).unwrap();
        let profile = image.profile();
        assert_eq!(image.passport(SupplierHardwareNumber), Some("NOTSUPPORT"));
        assert_eq!(image.passport(SupplierSoftwareNumber), Some(software));
        let (raw_software, _) = SupplierSoftwareNumber.slot(profile);
        let duplicate = raw_software + 0x2F;

        image
            .set_passport(SupplierHardwareNumber, "HW-123")
            .unwrap();
        image
            .set_passport(SupplierSoftwareNumber, "SW-123")
            .unwrap();
        assert!(image.undo());
        assert_eq!(image.passport(SupplierSoftwareNumber), Some(software));
        assert!(image.redo());
        let output = readback(&mut image);

        for (field, start, value) in [
            (SupplierHardwareNumber, hardware_reply, b"HW-123    "),
            (SupplierSoftwareNumber, hardware_reply + 17, b"SW-123    "),
        ] {
            let (raw, width) = field.slot(profile);
            let checksum = start + 5 + width;
            assert_eq!(&output[raw..raw + width], value);
            assert_eq!(output[raw + width], 0);
            assert_eq!(&output[start + 5..checksum], value);
            assert_eq!(output[checksum], sum(&output[start..checksum]) as u8);
            assert_eq!(output[checksum + 1], 0);
        }
        assert_eq!(&output[duplicate..duplicate + 10], software.as_bytes());
        let reopened = Image::new(output).unwrap();
        assert_eq!(reopened.passport(SupplierHardwareNumber), Some("HW-123"));
        assert_eq!(reopened.passport(SupplierSoftwareNumber), Some("SW-123"));
    }
}

#[test]
fn passport_rejects_invalid_text_and_malformed_slots() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    for value in ["123456789", "кириллица", "ab\n"] {
        assert!(
            image
                .set_passport(PassportField::SoftwareIdentifier, value)
                .is_err()
        );
    }
    assert!(!image.has_changes());
    let (address, width) = PassportField::EngineType.slot(image.profile());
    let mut malformed = SAMPLE.to_vec();
    malformed[address + width] = b'J';
    let mut image = Image::new(malformed).unwrap();
    assert_eq!(image.passport(PassportField::EngineType), None);
    assert!(
        image
            .set_passport(PassportField::EngineType, "SAMARA")
            .is_err()
    );
}

#[test]
fn checkbox_changes_one_bit_preserves_other_bits_and_survives_save() {
    for (stock, before) in [(SAMPLE.as_slice(), 0x2B), (I02, 0x08)] {
        let mut image = Image::new(stock.to_vec()).unwrap();
        let previous_value = image.flag(0x6051, 1).unwrap();
        image.set_flag(0x6051, 1, !previous_value).unwrap();
        assert_eq!(image.flag(0x6051, 1), Some(!previous_value));
        assert_eq!(image.saved_flag(0x6051, 1), Some(previous_value));
        assert!(image.undo());
        assert!(!image.has_changes());
        assert!(image.redo());
        let bin = readback(&mut image);
        assert_eq!(calibration_changes(stock, &bin), [0x6051]);
        assert_eq!(bin[0x6051], before ^ 0x02);
        let reopened = Image::new(bin).unwrap();
        assert_eq!(reopened.flag(0x6051, 1), Some(!previous_value));
    }
    let mut i02 = Image::new(I02.to_vec()).unwrap();
    assert!(i02.set_flag(0x5FB0, 1, true).is_err());
    assert!(i02.set_flag(0x6051, 8, true).is_err());
    assert!(i02.set_flag(0x6020, 1, true).is_err());
    assert!(!i02.has_changes());
}

#[test]
fn single_edits_round_and_clamp_to_the_representable_values() {
    let mut image = Image::new(SAMPLE.to_vec()).unwrap();
    for (parameter, index, input, expected) in [
        (Parameter::Ignition, 0, 21.26, 21.5),
        (Parameter::Ignition, 0, -25.25, -25.5),
        (Parameter::Ignition, 0, f64::MAX, 63.5),
        (Parameter::Ignition, 0, -f64::MAX, -64.0),
        (Parameter::FanOff, 0, 216.0, 215.0),
        (Parameter::FanOff, 0, -500.0, -40.0),
        (Parameter::Idle, 1, 956.0, 960.0),
    ] {
        image.set(parameter, index, input).unwrap();
        assert_eq!(image.value(parameter, index), Some(expected));
    }
}
