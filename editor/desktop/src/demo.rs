use janvar_editor::firmware::{IMAGE_SIZE, Profile};
use janvar_editor::j5::Catalog;

pub const SAMPLE: &[u8; IMAGE_SIZE] = include_bytes!("../../../bins/J5v05l19.bin");

pub fn reference(profile: Profile) -> Option<(Catalog, &'static str)> {
    let (data, name) = match profile {
        Profile::L19 => (
            include_bytes!("../../../bins/J5V05L19_738.j5").as_slice(),
            "J5V05L19_738.j5",
        ),
        Profile::I02 => (
            include_bytes!("../../../bins/J5V13I02_0738.j5").as_slice(),
            "J5V13I02_0738.j5",
        ),
        _ => return None,
    };
    Some((Catalog::parse(data).expect("bundled 0738 map"), name))
}
