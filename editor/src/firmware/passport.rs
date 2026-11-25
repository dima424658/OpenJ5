use super::{ByteEdit, CalibrationKind, Edit, FirmwareError, Image, Profile, sum};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum PassportField {
    HardwareNumber,
    EngineType,
    ProgrammingDate,
    SoftwareIdentifier,
    SupplierHardwareNumber,
    SupplierSoftwareNumber,
}

impl PassportField {
    pub const KIND: CalibrationKind = CalibrationKind::TextList;

    pub const ALL: [Self; 6] = [
        Self::HardwareNumber,
        Self::EngineType,
        Self::SoftwareIdentifier,
        Self::ProgrammingDate,
        Self::SupplierHardwareNumber,
        Self::SupplierSoftwareNumber,
    ];

    /// The start and width of the ASCII slot, excluding its zero terminator.
    pub fn slot(self, profile: Profile) -> (usize, usize) {
        let base = match profile {
            Profile::L19 => 0xC77D,
            Profile::I02 | Profile::G26 => 0xC400,
            Profile::Other => return (0, 0),
        };
        let (offset, width) = match self {
            Self::HardwareNumber => (0, 16),
            Self::EngineType => (0x27, 15),
            Self::ProgrammingDate => (0x37, 10),
            Self::SoftwareIdentifier => (0x42, 8),
            Self::SupplierHardwareNumber => (0x11, 10),
            Self::SupplierSoftwareNumber => (0x1C, 10),
        };
        (base + offset, width)
    }

    fn supplier_reply(self, profile: Profile, bytes: &[u8]) -> Option<usize> {
        let hardware_reply = match profile {
            Profile::L19 => 0xC8D8,
            Profile::I02 | Profile::G26 => 0xC53B,
            Profile::Other => return None,
        };
        let (start, id) = match self {
            Self::SupplierHardwareNumber => (hardware_reply, 0x92),
            Self::SupplierSoftwareNumber => (hardware_reply + 17, 0x94),
            _ => return None,
        };
        let header = [0x8C, 0xF1, 0x10, 0x5A, id];
        let end = start + header.len() + self.slot(profile).1;
        (bytes[start..].starts_with(&header) && bytes[end + 1] == 0).then_some(start)
    }

    fn text(self, profile: Profile, bytes: &[u8]) -> Option<&str> {
        let slot = self.slot(profile);
        if let Some(start) = self.supplier_reply(profile, bytes)
            && let Some(text) = ascii_text(&bytes[start + 5..start + 5 + slot.1])
        {
            return Some(text);
        }
        passport_text(bytes, slot)
    }
}

impl Image {
    pub fn passport(&self, field: PassportField) -> Option<&str> {
        field.text(self.profile, &self.data)
    }

    pub fn saved_passport(&self, field: PassportField) -> Option<&str> {
        field.text(self.profile, &self.saved)
    }

    pub fn set_passport(&mut self, field: PassportField, value: &str) -> Result<(), FirmwareError> {
        let slot = field.slot(self.profile);
        let (address, width) = slot;
        if self.passport(field).is_none() {
            return Err(FirmwareError::ReadOnly);
        }
        if value.len() > width || !printable(value.as_bytes()) {
            return Err(FirmwareError::InvalidPassportText { width });
        }
        let mut text = [b' '; 16];
        text[..value.len()].copy_from_slice(value.as_bytes());
        let mut edits = Vec::new();
        if passport_text(&self.data, slot).is_some() {
            append_edits(&mut edits, &self.data, address, &text[..width]);
        }
        if let Some(start) = field.supplier_reply(self.profile, &self.data) {
            let payload = start + 5;
            let checksum = sum(&self.data[start..payload]).wrapping_add(sum(&text[..width]));
            append_edits(&mut edits, &self.data, payload, &text[..width]);
            append_edits(&mut edits, &self.data, payload + width, &[checksum as u8]);
        }
        if !edits.is_empty() {
            self.commit(Edit::Calibration(edits));
        }
        Ok(())
    }
}

fn append_edits(edits: &mut Vec<ByteEdit>, bytes: &[u8], start: usize, payload: &[u8]) {
    edits.extend(payload.iter().enumerate().filter_map(|(index, &after)| {
        let address = start + index;
        let before = bytes[address];
        (before != after).then_some(ByteEdit {
            address,
            before,
            after,
        })
    }));
}

fn passport_text(bytes: &[u8], (address, width): (usize, usize)) -> Option<&str> {
    if width == 0 {
        return None;
    }
    (bytes[address + width] == 0)
        .then(|| ascii_text(&bytes[address..address + width]))
        .flatten()
}

fn ascii_text(field: &[u8]) -> Option<&str> {
    if printable(field) {
        std::str::from_utf8(field).ok().map(str::trim_end)
    } else {
        None
    }
}

fn printable(bytes: &[u8]) -> bool {
    bytes.iter().all(|byte| (b' '..=b'~').contains(byte))
}
