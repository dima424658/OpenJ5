use super::{ByteEdit, Edit, FirmwareError, Image, quantize_raw};
use std::collections::BTreeMap;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum Storage {
    U8,
    I8,
    U16Le,
    Bits(u8),
}

impl Storage {
    fn width(self) -> usize {
        if self == Self::U16Le { 2 } else { 1 }
    }

    fn bounds(self) -> (f64, f64) {
        match self {
            Self::U8 => (0.0, 255.0),
            Self::I8 => (-128.0, 127.0),
            Self::U16Le => (0.0, 65535.0),
            Self::Bits(mask) => (0.0, f64::from(mask >> mask.trailing_zeros())),
        }
    }

    fn read(self, bytes: &[u8]) -> f64 {
        match self {
            Self::U8 => f64::from(bytes[0]),
            Self::I8 => f64::from(bytes[0] as i8),
            Self::U16Le => f64::from(u16::from_le_bytes([bytes[0], bytes[1]])),
            Self::Bits(mask) => f64::from((bytes[0] & mask) >> mask.trailing_zeros()),
        }
    }
    fn write(self, raw: f64, original: u8) -> [u8; 2] {
        match self {
            Self::Bits(mask) => [
                (original & !mask) | ((raw as u8) << mask.trailing_zeros()),
                0,
            ],
            _ => (raw as i32 as u16).to_le_bytes(),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub(crate) struct Encoding {
    pub storage: Storage,
    pub scale: f64,
    pub zero: f64,
    pub physical: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, thiserror::Error)]
pub enum MappedError {
    #[error(
        "Адрес не найден. Калибровка может отсутствовать в этом ПО или иметь нестандартное расположение."
    )]
    AddressNotFound,
    #[error("Адрес найден, но формат значения ещё не разобран.")]
    UnsupportedEncoding,
}

/// Bounds-checked numeric record resolved against the open BIN.
#[derive(Clone, Debug)]
pub struct MapCalibration {
    start: usize,
    shape: Vec<usize>,
    software: [u8; 8],
    address_token: Option<u32>,
    count: usize,
    encoding: Encoding,
    writable: bool,
}

impl MapCalibration {
    pub(crate) fn resolve(
        image: &Image,
        address: Option<u32>,
        shape: &[usize],
        encoding: Option<Encoding>,
    ) -> Result<Self, MappedError> {
        let start = resolve_address(address, image.bytes()).ok_or(MappedError::AddressNotFound)?;
        let mut encoding = encoding.ok_or(MappedError::UnsupportedEncoding)?;
        if address == Some(0xFFFFFF96) {
            encoding = Encoding {
                storage: Storage::U8,
                scale: 1.0,
                zero: 0.0,
                physical: false,
            };
        }
        let count = shape
            .iter()
            .try_fold(1usize, |n, &length| n.checked_mul(length))
            .filter(|&n| n > 0)
            .ok_or(MappedError::AddressNotFound)?;
        let end = count
            .checked_mul(encoding.storage.width())
            .and_then(|size| start.checked_add(size))
            .filter(|&end| end <= image.bytes().len())
            .ok_or(MappedError::AddressNotFound)?;
        let writable = match address {
            Some(0xFFFFFC0C..=0xFFFFFC0E) => {
                start >= 0x4000
                    && end <= 0xFFF0
                    && !(start < 0x6050 && end > 0x600A)
                    && !(start < 0x5A25 && end > 0x5A23)
                    && !(start < 0xB6FF && end > 0xB6FD)
            }
            Some(0xFFFFFF96) => start >= 0x4000 && end <= 0xA000,
            _ => start >= 0x5F00 && end <= 0xA000 && !(start < 0x6050 && end > 0x600A),
        };
        Ok(Self {
            start,
            shape: shape.to_vec(),
            software: *image
                .software_id()
                .as_bytes()
                .first_chunk()
                .expect("validated software ID"),
            address_token: address.filter(|&value| value > 0xFFFF),
            count,
            encoding,
            writable,
        })
    }

    pub fn shape(&self) -> &[usize] {
        &self.shape
    }

    pub fn count(&self) -> usize {
        self.count
    }
    pub fn uses_physical_units(&self) -> bool {
        self.encoding.physical
    }
    pub fn writable(&self, image: &Image) -> bool {
        self.writable
            && image.can_edit_map()
            && self.compatible(image)
            && self
                .address_token
                .is_none_or(|token| resolve_address(Some(token), image.bytes()) == Some(self.start))
    }

    /// The physical range representable by one stored value.
    pub fn value_bounds(&self) -> (f64, f64) {
        let (min, max) = self.encoding.storage.bounds();
        let a = self.decode(min);
        let b = self.decode(max);
        (a.min(b), a.max(b))
    }

    fn decode(&self, raw: f64) -> f64 {
        (raw - self.encoding.zero) * self.encoding.scale
    }

    pub fn step(&self) -> f64 {
        self.encoding.scale.abs()
    }

    fn address(&self, index: usize) -> Option<usize> {
        (index < self.count).then(|| self.start + index * self.encoding.storage.width())
    }

    pub fn value(&self, image: &Image, index: usize) -> Option<f64> {
        self.compatible(image)
            .then(|| self.read(image.bytes(), index))
            .flatten()
    }

    pub fn saved_value(&self, image: &Image, index: usize) -> Option<f64> {
        self.compatible(image)
            .then(|| self.read(image.saved_bytes(), index))
            .flatten()
    }

    fn compatible(&self, image: &Image) -> bool {
        image.software_id().as_bytes() == self.software
    }

    pub fn byte_address(&self, index: usize) -> Option<usize> {
        self.address(index)
    }

    fn read(&self, bytes: &[u8], index: usize) -> Option<f64> {
        let address = self.address(index)?;
        let raw = self
            .encoding
            .storage
            .read(&bytes[address..address + self.encoding.storage.width()]);
        Some(self.decode(raw))
    }

    pub fn set(&self, image: &mut Image, index: usize, value: f64) -> Result<(), FirmwareError> {
        self.set_cells(image, [(index, value)])
    }

    /// Updates cells as one undoable edit. The last value for each index wins.
    pub fn set_cells(
        &self,
        image: &mut Image,
        cells: impl IntoIterator<Item = (usize, f64)>,
    ) -> Result<(), FirmwareError> {
        if !self.writable(image) {
            return Err(FirmwareError::ReadOnly);
        }
        let mut edits = Vec::new();
        for (index, value) in cells.into_iter().collect::<BTreeMap<_, _>>() {
            let address = self.address(index).ok_or(FirmwareError::InvalidIndex {
                index,
                count: self.count,
            })?;
            if !value.is_finite() {
                return Err(FirmwareError::NonFinite);
            }
            let raw = quantize_raw(
                value,
                self.encoding.scale,
                self.encoding.zero,
                self.encoding.storage.bounds(),
            );
            let packed = self.encoding.storage.write(raw, image.data[address]);
            for (offset, &after) in packed[..self.encoding.storage.width()].iter().enumerate() {
                let address = address + offset;
                let before = image.data[address];
                if before != after {
                    edits.push(ByteEdit {
                        address,
                        before,
                        after,
                    });
                }
            }
        }
        if !edits.is_empty() {
            image.commit(Edit::Calibration(edits));
        }
        Ok(())
    }
}

fn resolve_address(address: Option<u32>, bytes: &[u8]) -> Option<usize> {
    match address? {
        0xFFFFFF96 => (0x200..=0x6000).find_map(|offset| {
            let sample = bytes.get(offset - 2..offset + 9)?;
            matches!(
                &sample[2..],
                [0x93, 0xC3, 0xC2, 0xAF, 0x25, _, 0xC5, 0xF0, 0x05]
            )
            .then(|| usize::from(u16::from_be_bytes([sample[0], sample[1]])))
        }),
        token @ 0xFFFFFC0C..=0xFFFFFC0E => {
            let region = 0x4000..0xFFF0;
            let position = bytes
                .get(region.clone())?
                .windows(6)
                .position(|w| w == b"SMSSwS")?;
            Some(region.start + position + 6 + 39 * (0xFFFFFC0E - token) as usize)
        }
        direct @ 0..=0xFFFF => Some(direct as usize),
        _ => None,
    }
}
