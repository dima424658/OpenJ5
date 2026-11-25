use super::Profile;

// Each patch is surrounded by the same opcodes in the recognized variants.
const LOCATIONS: [(usize, &[u8; 3], &[u8; 4]); 4] = [
    (0x2530, b"\x51\xE4\x93", b"\xF5\x20\x90\x60"),
    (0x2A1F, b"\x51\xE4\x93", b"\xF5\x20\x90\x60"),
    (0x4D16, b"\xDA\xE4\x93", b"\x52\x5B\x90\x60"),
    (0x4F16, b"\xDA\xE4\x93", b"\x55\x5B\xF5\xF0"),
];

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RomMode {
    Stock,
    CtpDisabled,
    VendorDisabled,
    Unknown,
    Unsupported,
}

impl RomMode {
    pub(super) fn bytes(self) -> Option<[[u8; 2]; 4]> {
        match self {
            Self::Stock => Some([[0x44, 0x01], [0x44, 0x01], [0x44, 0x80], [0x44, 0x80]]),
            Self::CtpDisabled => Some([[0x00, 0x00], [0x00, 0x00], [0x54, 0x7F], [0x54, 0x7F]]),
            Self::VendorDisabled => Some([[0x44, 0x00]; 4]),
            Self::Unknown | Self::Unsupported => None,
        }
    }
}

pub(super) fn detect(profile: Profile, data: &[u8]) -> RomMode {
    if profile != Profile::L19 {
        return RomMode::Unsupported;
    }
    for (offset, before, after) in LOCATIONS {
        if &data[offset - 3..offset] != before || &data[offset + 2..offset + 6] != after {
            return RomMode::Unknown;
        }
    }
    [
        RomMode::Stock,
        RomMode::CtpDisabled,
        RomMode::VendorDisabled,
    ]
    .into_iter()
    .find(|mode| {
        let patches = mode.bytes().expect("known ROM mode");
        LOCATIONS
            .into_iter()
            .zip(patches)
            .all(|((offset, _, _), expected)| data[offset..offset + 2] == expected)
    })
    .unwrap_or(RomMode::Unknown)
}

pub(super) fn write(data: &mut [u8], mode: RomMode) {
    let patches = mode.bytes().expect("known ROM mode");
    for ((address, _, _), patch) in LOCATIONS.into_iter().zip(patches) {
        data[address..address + 2].copy_from_slice(&patch);
    }
}
