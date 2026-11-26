mod calibration;
mod mapped;
mod passport;
mod rom;

pub use calibration::{Batch, CalibrationKind, Parameter, Spec, TableCell, idle_temperature};
pub(crate) use mapped::{Encoding, Storage};
pub use mapped::{MapCalibration, MappedError};
pub use passport::PassportField;
pub use rom::RomMode;

use std::collections::BTreeMap;
use std::fs;
use std::io::Write;
use std::path::Path;

use thiserror::Error;

pub const IMAGE_SIZE: usize = 0x10000;
const FLASH_SUM: usize = 0xFFFE;
const FLASH_COMPLEMENT: usize = 0xFFFA;

#[derive(Debug, Error)]
pub enum FirmwareError {
    #[error("Ожидался BIN размером 65 536 байт. Получено {actual} байт.")]
    InvalidSize { actual: usize },
    #[error("Ожидался образ Январь 5.1 с идентификатором ПО.")]
    UnsupportedProfile,
    #[error("Смещения в BIN отличаются от известных для этого профиля.")]
    InvalidLayout,
    #[error("В BIN L19 не найдены известные ссылки на контрольные суммы калибровок.")]
    InvalidChecksumLayout,
    #[error("Ячейка {index} находится за пределами калибровки из {count} значений.")]
    InvalidIndex { index: usize, count: usize },
    #[error("Эта калибровка доступна для чтения.")]
    ReadOnly,
    #[error("Введите конечное число.")]
    NonFinite,
    #[error("В BIN не распознана схема защиты калибровок L19.")]
    UnknownRomControl,
    #[error("Поле паспорта принимает до {width} печатных символов ASCII.")]
    InvalidPassportText { width: usize },
    #[error("Неизвестное смещение или номер флага.")]
    InvalidFlag,
    #[error("Не удалось прочитать BIN. {0}")]
    Read(#[from] std::io::Error),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SaveMode {
    /// Fail if another file is already present at the destination.
    Create,
    /// Atomically replace an existing file, or create a new one.
    Replace,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Profile {
    L19,
    I02,
    G26,
    Other,
}

impl Profile {
    pub fn id(self) -> &'static str {
        match self {
            Self::L19 => "J5V05L19",
            Self::I02 => "J5V13I02",
            Self::G26 => "J5V07G26",
            Self::Other => "Январь 5.1",
        }
    }

    pub fn hardware(self) -> &'static str {
        match self {
            Self::L19 => "2112-1411020-41",
            Self::I02 => "2111-1411020-71",
            Self::G26 => "2112-1411020-71",
            Self::Other => "Январь 5.1",
        }
    }

    pub fn detect(bytes: &[u8]) -> Result<Self, FirmwareError> {
        if bytes.len() != IMAGE_SIZE {
            return Err(FirmwareError::InvalidSize {
                actual: bytes.len(),
            });
        }
        let profile = match &bytes[0x6020..0x6028] {
            b"J5V05L19" if bytes.starts_with(b"\x02\x20\x4D\x02\x07\x8C") => Self::L19,
            b"J5V13I02" if bytes.starts_with(b"\x02\x10\x2F\x02\x07\x45") => Self::I02,
            b"J5V07G26" if bytes.starts_with(b"\x02\x13\x52\x02\x07\x45") => Self::G26,
            b"J5V05L19" | b"J5V13I02" | b"J5V07G26" => return Err(FirmwareError::InvalidLayout),
            id if &bytes[0x600A..0x600E] == b"J5.1"
                && id.starts_with(b"J5")
                && id.iter().all(u8::is_ascii_alphanumeric) =>
            {
                return Ok(Self::Other);
            }
            _ => return Err(FirmwareError::UnsupportedProfile),
        };
        let ignition_call = match profile {
            Self::L19 => 0x3F95,
            Self::I02 => 0x2657,
            Self::G26 => 0x2AA5,
            Self::Other => unreachable!("generic profile returns above"),
        };
        if &bytes[0x600A..0x600E] != b"J5.1"
            || &bytes[ignition_call..ignition_call + 3] != b"\x90\x6A\xC8"
        {
            return Err(FirmwareError::InvalidLayout);
        }
        if profile == Self::G26
            && (&bytes[0xE3F0..0xE3FA] != b"\xEB\x90\xFF\xFE\xE4\x93\xC3\x98\xF8\xA3")
        {
            return Err(FirmwareError::InvalidChecksumLayout);
        }
        if profile == Self::L19
            && (&bytes[0x5317..0x531C] != b"\x90\x5A\x23\xE4\x93"
                || &bytes[0x5398..0x539D] != b"\x90\xB6\xFD\xE4\x93")
        {
            return Err(FirmwareError::InvalidChecksumLayout);
        }
        Ok(profile)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ChecksumKind {
    Flash,
    FlashComplement,
    Calibration6000,
    Calibration8000,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Checksum {
    pub kind: ChecksumKind,
    pub stored: u16,
    pub calculated: u16,
}

impl Checksum {
    pub fn valid(self) -> bool {
        self.stored == self.calculated
    }
}

/// Current and last saved representations of a node in the BIN image.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Reading {
    pub address: usize,
    pub raw: u8,
    pub saved_raw: u8,
    pub value: f64,
    pub saved_value: f64,
}

struct ByteEdit {
    address: usize,
    before: u8,
    after: u8,
}

enum Edit {
    Calibration(Vec<ByteEdit>),
    Rom { before: RomMode, after: RomMode },
}

pub struct Image {
    profile: Profile,
    data: Vec<u8>,
    saved: Vec<u8>,
    checksums: Vec<Checksum>,
    undo: Vec<Edit>,
    redo: Vec<Edit>,
}

impl Image {
    pub fn new(data: Vec<u8>) -> Result<Self, FirmwareError> {
        let profile = Profile::detect(&data)?;
        Ok(Self {
            profile,
            checksums: checksums(profile, &data),
            saved: data.clone(),
            data,
            undo: Vec::new(),
            redo: Vec::new(),
        })
    }

    pub fn profile(&self) -> Profile {
        self.profile
    }

    pub fn software_id(&self) -> &str {
        std::str::from_utf8(&self.data[0x6020..0x6028]).expect("validated firmware identifier")
    }

    pub fn can_edit_map(&self) -> bool {
        self.profile != Profile::Other
    }

    /// Bytes of the working image. Use [Self::to_bin] to export an image with updated checksums.
    pub fn bytes(&self) -> &[u8] {
        &self.data
    }

    pub fn saved_bytes(&self) -> &[u8] {
        &self.saved
    }

    /// Reads the decoded value, returning `None` for a node outside the calibration.
    pub fn value(&self, parameter: Parameter, index: usize) -> Option<f64> {
        if !self.can_edit_map() {
            return None;
        }
        let address = parameter.spec().address;
        (index < parameter.count()).then(|| parameter.decode(self.data[address + index]))
    }

    pub fn reading(&self, parameter: Parameter, index: usize) -> Option<Reading> {
        if !self.can_edit_map() || index >= parameter.count() {
            return None;
        }
        let address = parameter.spec().address + index;
        let raw = self.data[address];
        let saved_raw = self.saved[address];
        Some(Reading {
            address,
            raw,
            saved_raw,
            value: parameter.decode(raw),
            saved_value: parameter.decode(saved_raw),
        })
    }

    /// Reads a flag from the known configuration and diagnostic banks.
    pub fn flag(&self, address: u16, bit: u8) -> Option<bool> {
        self.flag_mask(address, bit)
            .map(|mask| self.data[usize::from(address)] & mask != 0)
    }

    pub fn saved_flag(&self, address: u16, bit: u8) -> Option<bool> {
        self.flag_mask(address, bit)
            .map(|mask| self.saved[usize::from(address)] & mask != 0)
    }

    pub fn set_flag(&mut self, address: u16, bit: u8, checked: bool) -> Result<(), FirmwareError> {
        if !self.can_edit_map() {
            return Err(FirmwareError::ReadOnly);
        }
        let mask = self
            .flag_mask(address, bit)
            .ok_or(FirmwareError::InvalidFlag)?;
        let address = usize::from(address);
        let before = self.data[address];
        let after = if checked {
            before | mask
        } else {
            before & !mask
        };
        if before != after {
            self.commit(Edit::Calibration(vec![ByteEdit {
                address,
                before,
                after,
            }]));
        }
        Ok(())
    }

    fn flag_mask(&self, address: u16, bit: u8) -> Option<u8> {
        let known_bank = matches!(address, 0x6051..=0x6052 | 0x60DA..=0x60DE)
            || self.profile == Profile::L19 && matches!(address, 0x5FAC | 0x5FB0..=0x5FB1);
        (known_bank && bit < 8).then(|| 1 << bit)
    }

    pub fn rpm(&self, index: usize) -> Option<u16> {
        (index < 16).then(|| word(&self.data, 0x623C + index * 2))
    }

    pub fn load_axis(&self, index: usize) -> Option<u32> {
        if index >= 16 {
            return None;
        }
        let factor = f64::from(self.data[0x6066]);
        if factor == 0.0 {
            return None;
        }
        let step = (256.0 / (6.0 * factor / 8.0) * 16.0).round() as u32;
        Some(u32::from(word(&self.data, 0x6064)) / 6 + step * index as u32)
    }

    pub fn set(
        &mut self,
        parameter: Parameter,
        index: usize,
        value: f64,
    ) -> Result<(), FirmwareError> {
        self.set_cells(parameter, [(index, value)])
    }

    /// Updates cells as one undoable edit. The last value for each index wins.
    pub fn set_cells(
        &mut self,
        parameter: Parameter,
        cells: impl IntoIterator<Item = (usize, f64)>,
    ) -> Result<(), FirmwareError> {
        if !self.can_edit_map() {
            return Err(FirmwareError::ReadOnly);
        }
        let mut edits = Vec::new();
        for (index, value) in cells.into_iter().collect::<BTreeMap<_, _>>() {
            if index >= parameter.count() {
                return Err(FirmwareError::InvalidIndex {
                    index,
                    count: parameter.count(),
                });
            }
            let after = parameter.encode(value)?;
            let address = parameter.spec().address + index;
            let before = self.data[address];
            if before != after {
                edits.push(ByteEdit {
                    address,
                    before,
                    after,
                });
            }
        }
        if !edits.is_empty() {
            self.commit(Edit::Calibration(edits));
        }
        Ok(())
    }

    pub fn apply_batch(
        &mut self,
        parameter: Parameter,
        operation: Batch,
        input: f64,
    ) -> Result<(), FirmwareError> {
        if !self.can_edit_map() {
            return Err(FirmwareError::ReadOnly);
        }
        if !input.is_finite() {
            return Err(FirmwareError::NonFinite);
        }
        let spec = parameter.spec();
        let mut edits = Vec::new();
        for index in 0..parameter.count() {
            let current = parameter.decode(self.data[spec.address + index]);
            let target = match operation {
                Batch::Set => input,
                Batch::Shift => current + input,
                Batch::Percent => current * (1.0 + input / 100.0),
            };
            let after = parameter.encode(target.clamp(spec.min, spec.max))?;
            let address = spec.address + index;
            let before = self.data[address];
            if before != after {
                edits.push(ByteEdit {
                    address,
                    before,
                    after,
                });
            }
        }
        if !edits.is_empty() {
            self.commit(Edit::Calibration(edits));
        }
        Ok(())
    }

    pub fn rom_mode(&self) -> RomMode {
        rom::detect(self.profile, &self.data)
    }

    pub fn saved_rom_mode(&self) -> RomMode {
        rom::detect(self.profile, &self.saved)
    }

    pub fn set_rom_control(&mut self, enabled: bool) -> Result<(), FirmwareError> {
        let before = self.rom_mode();
        let after = match (before, enabled) {
            (RomMode::Unknown | RomMode::Unsupported, _) => {
                return Err(FirmwareError::UnknownRomControl);
            }
            (RomMode::Stock, false) => RomMode::CtpDisabled,
            (_, true) => RomMode::Stock,
            (_, false) => before,
        };
        if before != after {
            self.commit(Edit::Rom { before, after });
        }
        Ok(())
    }

    fn commit(&mut self, edit: Edit) {
        self.apply(&edit, true);
        self.undo.push(edit);
        self.redo.clear();
    }

    fn apply(&mut self, edit: &Edit, forward: bool) {
        match edit {
            Edit::Calibration(edits) => {
                for edit in edits {
                    self.data[edit.address] = if forward { edit.after } else { edit.before };
                }
            }
            Edit::Rom { before, after } => {
                rom::write(&mut self.data, if forward { *after } else { *before });
            }
        }
        self.checksums = checksums(self.profile, &self.data);
    }

    pub fn undo(&mut self) -> bool {
        if let Some(edit) = self.undo.pop() {
            self.apply(&edit, false);
            self.redo.push(edit);
            return true;
        }
        false
    }

    pub fn redo(&mut self) -> bool {
        if let Some(edit) = self.redo.pop() {
            self.apply(&edit, true);
            self.undo.push(edit);
            return true;
        }
        false
    }

    pub fn can_undo(&self) -> bool {
        !self.undo.is_empty()
    }

    pub fn can_redo(&self) -> bool {
        !self.redo.is_empty()
    }

    pub fn differences(&self) -> impl Iterator<Item = (Parameter, usize)> + '_ {
        Parameter::ALL.into_iter().flat_map(move |param| {
            let base = param.spec().address;
            (0..param.count())
                .filter(move |index| self.data[base + index] != self.saved[base + index])
                .map(move |index| (param, index))
        })
    }

    pub fn has_changes(&self) -> bool {
        self.data != self.saved
    }

    pub fn checksums(&self) -> &[Checksum] {
        &self.checksums
    }

    /// Writes an updated BIN atomically and updates the saved baseline on success.
    pub fn save(&mut self, path: &Path, mode: SaveMode) -> std::io::Result<()> {
        if !self.can_edit_map() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::Unsupported,
                FirmwareError::ReadOnly,
            ));
        }
        let output = self.to_bin();
        let directory = path
            .parent()
            .filter(|path| !path.as_os_str().is_empty())
            .unwrap_or_else(|| Path::new("."));
        let mut temporary = tempfile::NamedTempFile::new_in(directory)?;
        temporary.write_all(&output)?;
        temporary.as_file().sync_all()?;
        match mode {
            SaveMode::Create => temporary.persist_noclobber(path)?,
            SaveMode::Replace => temporary.persist(path)?,
        };
        self.saved.clone_from(&output);
        self.data = output;
        self.checksums = checksums(self.profile, &self.data);
        Ok(())
    }

    /// Exports the current image with recalculated checksums, without changing the edit state.
    pub fn to_bin(&self) -> Vec<u8> {
        let mut output = self.data.clone();
        if !self.can_edit_map() {
            return output;
        }
        if self.profile == Profile::L19 {
            let first = sum(&output[0x6000..0x8000]);
            let second = sum(&output[0x8000..0xA000]);
            put_word(&mut output, 0x5A23, first);
            put_word(&mut output, 0xB6FD, second);
        }
        update_flash_checksum(
            output
                .as_mut_slice()
                .try_into()
                .expect("validated Flash size"),
        );
        output
    }
}

fn quantize_raw(value: f64, scale: f64, zero: f64, (min, max): (f64, f64)) -> f64 {
    (value / scale + zero).round().clamp(min, max)
}

pub fn open(path: &Path) -> Result<Image, FirmwareError> {
    Image::new(fs::read(path)?)
}

fn word(bytes: &[u8], address: usize) -> u16 {
    u16::from_le_bytes([bytes[address], bytes[address + 1]])
}

fn put_word(bytes: &mut [u8], address: usize, value: u16) {
    bytes[address..address + 2].copy_from_slice(&value.to_le_bytes());
}

fn sum(bytes: &[u8]) -> u16 {
    bytes
        .iter()
        .fold(0u16, |acc, &byte| acc.wrapping_add(u16::from(byte)))
}

fn flash_sum(bytes: &[u8]) -> u16 {
    sum(&bytes[..FLASH_COMPLEMENT])
        .wrapping_add(sum(&bytes[0xFFFC..FLASH_SUM]))
        .wrapping_add(0x01FE)
}

/// Updates the complementary Flash checksum words without changing calibrations.
pub fn update_flash_checksum(bytes: &mut [u8; IMAGE_SIZE]) {
    let total = flash_sum(bytes);
    put_word(bytes, FLASH_COMPLEMENT, !total);
    put_word(bytes, FLASH_SUM, total);
}

fn checksums(profile: Profile, bytes: &[u8]) -> Vec<Checksum> {
    let total = flash_sum(bytes);
    let mut result = vec![
        Checksum {
            kind: ChecksumKind::Flash,
            stored: word(bytes, FLASH_SUM),
            calculated: total,
        },
        Checksum {
            kind: ChecksumKind::FlashComplement,
            stored: word(bytes, FLASH_COMPLEMENT),
            calculated: !total,
        },
    ];
    if profile == Profile::L19 {
        result.extend([
            Checksum {
                kind: ChecksumKind::Calibration6000,
                stored: word(bytes, 0x5A23),
                calculated: sum(&bytes[0x6000..0x8000]),
            },
            Checksum {
                kind: ChecksumKind::Calibration8000,
                stored: word(bytes, 0xB6FD),
                calculated: sum(&bytes[0x8000..0xA000]),
            },
        ]);
    }
    result
}

#[cfg(test)]
mod tests;
