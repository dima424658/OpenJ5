use super::*;
use crate::firmware::Profile;

const L19_MAPS: &[(&[u8], Build)] = &[
    (
        include_bytes!("../../../bins/J5V05L19_738.j5"),
        Build::Ctp0738,
    ),
    (
        include_bytes!("../../../bins/J5V05L19_738_f.j5"),
        Build::Ctp0738,
    ),
    (
        include_bytes!("../../../bins/J5v05l19_0923.j5"),
        Build::Ctp0923,
    ),
    (
        include_bytes!("../../../bins/J5V05L19_0923_f.j5"),
        Build::Ctp0923,
    ),
];
const I02_MAPS: &[(&[u8], Build)] = &[
    (
        include_bytes!("../../../bins/J5V13I02_0738.j5"),
        Build::Ctp0738,
    ),
    (
        include_bytes!("../../../bins/J5V13I02_0738_f.j5"),
        Build::Ctp0738,
    ),
    (
        include_bytes!("../../../bins/J5v13i02_0923.j5"),
        Build::Ctp0923,
    ),
    (
        include_bytes!("../../../bins/J5V13I02_0923_f.j5"),
        Build::Ctp0923,
    ),
];

fn matched(bytes: &[u8], profile: Profile) -> Result<Catalog, MapError> {
    let fixture = match profile {
        Profile::L19 => L19_MAPS[0].0,
        Profile::I02 => I02_MAPS[0].0,
        _ => unreachable!("unsupported fixture"),
    };
    let reference = Catalog::parse(fixture).unwrap();
    Catalog::parse(bytes)?.with_reference(&reference)
}

#[test]
fn identifies_build_and_record_layout_in_all_eight_files() {
    for (profile, fixtures, count) in [(Profile::L19, L19_MAPS, 317), (Profile::I02, I02_MAPS, 255)]
    {
        for (index, &(bytes, build)) in fixtures.iter().enumerate() {
            let map = matched(bytes, profile).unwrap();
            assert_eq!(map.build(), build);
            let expected =
                count + usize::from(profile == Profile::L19 && build == Build::Ctp0923) * 2;
            assert_eq!(map.entries.len(), expected);
            for parameter in Parameter::ALL {
                let entry = &map.entries[map.find(parameter).unwrap()];
                assert!(!entry.path.is_empty());
                assert_eq!(entry.parameter(), Some(parameter));
            }
            let angle = &map.entries[map.find(Parameter::Ignition).unwrap()];
            assert_eq!(angle.title.as_deref(), Some("УОЗ для экономичного режима"));
            let rpm_tag = if index % 2 == 0 {
                "!002-613C-30"
            } else {
                "!002-613C-40"
            };
            assert_eq!(angle.tags, [rpm_tag, "!004-6064-6066"]);
        }
    }
}

#[test]
fn build_comes_from_ctm_header_even_if_filename_misleads() {
    let directory = tempfile::tempdir().unwrap();
    let path = directory.path().join("renamed_0738.j5");
    std::fs::write(&path, L19_MAPS[2].0).unwrap();
    let map = Catalog::parse(&std::fs::read(path).unwrap()).unwrap();
    assert_eq!(map.build(), Build::Ctp0923);
}

#[test]
fn leaves_unmatched_0923_fields_unresolved() {
    let map = matched(L19_MAPS[2].0, Profile::L19).unwrap();
    let entry = map
        .entries
        .iter()
        .find(|entry| entry.id == [0x12, 0x3C, 0xF7, 0xB3])
        .unwrap();
    assert_eq!(entry.units, "об/мин");
    assert!(entry.title.is_none());
    assert!(entry.path.is_empty());
    assert_eq!(entry.address, None);
    assert_eq!(entry.parameter(), None);
}

#[test]
fn refuses_cross_family_maps_in_both_directions() {
    for &(bytes, _) in L19_MAPS {
        assert!(matched(bytes, Profile::I02).is_err());
    }
    for &(bytes, _) in I02_MAPS {
        assert!(matched(bytes, Profile::L19).is_err());
    }
}

#[test]
fn rejects_unknown_signature_and_truncated_records() {
    let bytes = L19_MAPS[0].0;
    let mut wrong_magic = bytes.to_vec();
    wrong_magic[4] = 0xFF;
    assert!(parse(b"").is_err());
    assert!(parse(&wrong_magic).is_err());
    for ends in [5, 0x2EA, 0x2EA + 8, bytes.len() - 1] {
        assert!(parse(&bytes[..ends]).is_err());
    }
}

#[test]
fn refuses_write_access_when_0738_address_differs() {
    let mut map = L19_MAPS[0].0.to_vec();
    let offset = parse(&map)
        .unwrap()
        .entries
        .into_iter()
        .find(|entry| entry.parameter() == Some(Parameter::FanOn))
        .unwrap()
        .offset;
    map[offset + 0x45 + 4] ^= 1;
    assert!(matches!(
        matched(&map, Profile::L19),
        Err(MapError::IncompatibleMap)
    ));
}

#[test]
fn rejects_a_0923_reference_instead_of_inventing_missing_fields() {
    let reference = Catalog::parse(L19_MAPS[2].0).unwrap();
    let catalog = Catalog::parse(L19_MAPS[3].0).unwrap();
    assert!(matches!(
        catalog.with_reference(&reference),
        Err(MapError::InvalidReference)
    ));
}

#[test]
fn checkbox_labels_and_addresses_match_between_all_map_variants() {
    for fixtures in [L19_MAPS, I02_MAPS] {
        let reference = Catalog::parse(fixtures[0].0).unwrap();
        for &(data, _) in fixtures {
            let map = Catalog::parse(data)
                .unwrap()
                .with_reference(&reference)
                .unwrap();
            let flags: Vec<_> = map
                .entries()
                .iter()
                .filter(|entry| !entry.flags.is_empty())
                .collect();
            assert_eq!(flags.len(), if fixtures == L19_MAPS { 3 } else { 2 });
            let configuration = flags
                .iter()
                .find(|entry| entry.identifier() == "238E7426")
                .unwrap();
            assert_eq!(
                configuration.calibration_kind(),
                Some(CalibrationKind::CheckboxList)
            );
            let oxygen = &configuration.flags[0];
            assert_eq!(
                (oxygen.address, oxygen.bit, oxygen.title.as_str()),
                (0x6051, 0, "Датчик концентрации кислорода")
            );
            let error_mask = flags
                .iter()
                .find(|entry| entry.identifier() == "26D7E7E1")
                .unwrap();
            assert!(error_mask.flags.iter().any(|flag| {
                flag.address == 0x60DA && flag.bit == 7 && flag.title == "Ошибка ПЗУ"
            }));
        }
    }
}

#[test]
fn mapped_calibrations_read_write_and_round_trip_with_each_build() {
    use crate::firmware::{Image, MappedError};
    let stock = include_bytes!("../../../bins/J5v05l19.bin");
    for &(data, _) in L19_MAPS {
        let catalog = matched(data, Profile::L19).unwrap();
        let mut image = Image::new(stock.to_vec()).unwrap();
        let lookup = |name: &str| {
            catalog
                .entries()
                .iter()
                .find(|entry| entry.title.as_deref() == Some(name))
                .unwrap()
        };
        let pump = lookup("Время работы бензонасоса до пуска")
            .numeric(&image)
            .unwrap();
        assert_eq!(pump.value(&image, 0), Some(3.0));
        pump.set(&mut image, 0, 5.0).unwrap();
        assert_eq!(image.bytes()[0x606E], 5);
        let angle = lookup("УОЗ для экономичного режима")
            .numeric(&image)
            .unwrap();
        assert_eq!(angle.value(&image, 17), Some(21.0));
        angle.set(&mut image, 17, -25.5).unwrap();
        assert_eq!(image.bytes()[0x6AD9], 0xCD);
        let idle = lookup("Желаемые обороты");
        assert_eq!(idle.axis_value(&image, 0, 24), Some(80.0));
        let idle = idle.numeric(&image).unwrap();
        idle.set(&mut image, 24, 910.0).unwrap();
        assert_eq!(image.bytes()[0x6F56 + 24], 91);
        let reopened = Image::new(image.to_bin()).unwrap();
        assert!(reopened.checksums().iter().all(|sum| sum.valid()));

        let mut missing = 0;
        for entry in catalog
            .entries()
            .iter()
            .filter(|entry| entry.kind != Kind::Bitfield)
        {
            if let Err(error) = entry.numeric(&image) {
                assert_eq!(error, MappedError::AddressNotFound, "{:?}", entry.title);
                assert!(matches!(
                    entry.address,
                    Some(0xFFFFFC0C..=0xFFFFFC0E) | None
                ));
                missing += 1;
            }
        }
        assert_eq!(
            missing,
            3 + usize::from(catalog.build() == Build::Ctp0923) * 2
        );
    }
}

#[test]
fn packed_tables_preserve_other_fields_in_the_same_byte() {
    use crate::firmware::Image;
    let catalog = matched(L19_MAPS[0].0, Profile::L19).unwrap();
    let stock = include_bytes!("../../../bins/J5v05l19.bin");
    let mut image = Image::new(stock.to_vec()).unwrap();
    let find = |title| {
        catalog
            .entries()
            .iter()
            .find(|entry| entry.title.as_deref() == Some(title))
            .unwrap()
            .numeric(&image)
            .unwrap()
    };
    let knock = find("Зона контроля детонации");
    let lambda = find("Зона регулирования");
    let cell = (0..knock.count())
        .find(|&i| image.bytes()[0x63D3 + i] == 5)
        .unwrap();
    assert_eq!(knock.value(&image, cell), Some(1.0));
    assert_eq!(lambda.value(&image, cell), Some(1.0));
    lambda.set(&mut image, cell, 0.0).unwrap();
    assert_eq!(image.bytes()[0x63D3 + cell], 1);
    assert_eq!(knock.value(&image, cell), Some(1.0));
    assert!(image.undo());
    assert_eq!(image.bytes()[0x63D3 + cell], 5);
}

#[test]
fn special_address_resolution_uses_bin_content_for_every_entry() {
    use crate::firmware::{Image, MappedError};
    let catalog = matched(L19_MAPS[0].0, Profile::L19).unwrap();
    let stock = include_bytes!("../../../bins/J5v05l19.bin");
    let absent = Image::new(stock.to_vec()).unwrap();
    let mut data = stock.to_vec();
    data[0xA100..0xA106].copy_from_slice(b"SMSSwS");
    let present = Image::new(data).unwrap();
    for entry in catalog.entries() {
        if let Some(token @ 0xFFFFFC0C..=0xFFFFFC0E) = entry.address {
            assert!(matches!(
                entry.numeric(&absent),
                Err(MappedError::AddressNotFound)
            ));
            assert_eq!(
                entry.numeric(&present).unwrap().byte_address(0),
                Some(0xA106 + 39 * (0xFFFFFC0E - token) as usize)
            );
        }
    }
    let dmr = catalog
        .entries()
        .iter()
        .find(|entry| entry.address == Some(0xFFFFFF96))
        .unwrap();
    assert_eq!(dmr.numeric(&absent).unwrap().byte_address(0), Some(0x99B9));
}

#[test]
fn missing_addresses_are_reported_independently_of_the_entry_title() {
    use crate::firmware::{Image, MappedError};
    let stock = include_bytes!("../../../bins/J5v13i02.bin");
    let image = Image::new(stock.to_vec()).unwrap();
    let mut map = matched(I02_MAPS[0].0, Profile::I02).unwrap();
    let entry = map
        .entries
        .iter_mut()
        .find(|entry| entry.kind == Kind::Table)
        .unwrap();
    for address in [None, Some(0xFFFFFFAB), Some(0xFFFE)] {
        entry.address = address;
        assert!(matches!(
            entry.numeric(&image),
            Err(MappedError::AddressNotFound)
        ));
    }
}

#[test]
fn l19_startup_fuel_curves_match_ctp_graphs() {
    use crate::firmware::Image;

    let image = Image::new(include_bytes!("../../../bins/J5v05l19.bin").to_vec()).unwrap();
    let samples = [
        (
            [0x21, 0xD3, 0x5B, 0xE1],
            [0, 0, 144, 128, 87, 57, 38, 26, 18, 17, 24, 24, 24],
            1.4222,
        ),
        (
            [0x75, 0x92, 0x95, 0x6F],
            [321, 282, 191, 131, 82, 63, 48, 37, 26, 20, 23, 31, 31],
            2.84439215,
        ),
    ];
    for &(bytes, _) in L19_MAPS {
        let catalog = matched(bytes, Profile::L19).unwrap();
        for (id, expected, step) in samples {
            let entry = catalog
                .entries()
                .iter()
                .find(|entry| entry.id == id)
                .unwrap();
            assert!(entry.axis_label(0).unwrap().starts_with("Температура"));
            assert!(entry.units.ends_with("мг/цикл"));
            let calibration = entry.numeric(&image).unwrap();
            assert_eq!(calibration.count(), 39);
            assert_eq!(entry.axis_value(&image, 0, 38), Some(150.0));
            assert_eq!(calibration.value_bounds(), (0.0, 255.0 * step));
            for (index, expected_value) in (0..39).step_by(3).zip(expected) {
                assert_eq!(
                    entry.axis_value(&image, 0, index),
                    Some(-40.0 + 5.0 * index as f64)
                );
                assert_eq!(
                    calibration.value(&image, index).unwrap().round(),
                    f64::from(expected_value)
                );
            }
        }
    }
}

#[test]
fn warmup_ignition_curve_pairs_temperature_with_signed_correction() {
    use crate::firmware::Image;

    let image = Image::new(include_bytes!("../../../bins/J5v05l19.bin").to_vec()).unwrap();
    let catalog = matched(L19_MAPS[0].0, Profile::L19).unwrap();
    let entry = catalog
        .entries()
        .iter()
        .find(|entry| entry.id == [0xA2, 0xB1, 0x4A, 0xBB])
        .unwrap();
    let calibration = entry.numeric(&image).unwrap();
    for (index, temp, correction) in [
        (0, -40.0, 0.0),
        (11, 15.0, 0.0),
        (12, 20.0, -12.0),
        (17, 45.0, -11.0),
        (24, 80.0, 0.0),
    ] {
        assert_eq!(entry.axis_value(&image, 0, index), Some(temp));
        assert_eq!(calibration.value(&image, index), Some(correction));
    }
}

#[test]
fn mapped_edits_round_and_clamp_before_writing() {
    use crate::firmware::Image;

    let mut image = Image::new(include_bytes!("../../../bins/J5v05l19.bin").to_vec()).unwrap();
    let map = matched(L19_MAPS[0].0, Profile::L19).unwrap();
    let ignition = &map.entries()[map.find(Parameter::Ignition).unwrap()];
    let ignition = ignition.numeric(&image).unwrap();
    for (input, expected) in [(-25.3, -25.5), (99.2, 63.5), (-90.7, -64.0)] {
        ignition.set(&mut image, 17, input).unwrap();
        assert_eq!(ignition.value(&image, 17), Some(expected));
    }
    let fan = &map.entries()[map.find(Parameter::FanOn).unwrap()];
    let fan = fan.numeric(&image).unwrap();
    fan.set(&mut image, 0, 100.9).unwrap();
    assert_eq!(fan.value(&image, 0), Some(101.0));
    fan.set(&mut image, 0, -100.0).unwrap();
    assert_eq!(fan.value(&image, 0), Some(-40.0));
    assert!(fan.set(&mut image, 0, f64::INFINITY).is_err());
    assert_eq!(fan.value(&image, 0), Some(-40.0));
}
