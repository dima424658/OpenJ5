use crate::firmware::{
    CalibrationKind, Encoding, Image, MapCalibration, MappedError, Parameter, Storage,
};
use encoding_rs::WINDOWS_1251;
use std::collections::HashMap;
use std::fmt;

use thiserror::Error;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Kind {
    Folder,
    Curve,
    Table,
    Scalar,
    Bitfield,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Build {
    Ctp0738,
    Ctp0923,
}

impl Build {
    fn title(self) -> &'static str {
        match self {
            Self::Ctp0738 => "0738",
            Self::Ctp0923 => "0923",
        }
    }
}

impl fmt::Display for Build {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        formatter.write_str(self.title())
    }
}

#[derive(Debug, Error)]
pub enum MapError {
    #[error("Ожидалась карта CTM сборки 0738 или 0923.")]
    UnsupportedFormat,
    #[error("Некорректная запись карты по смещению 0x{offset:X}.")]
    InvalidRecord { offset: usize },
    #[error("В карте нет калибровок.")]
    Empty,
    #[error("В качестве эталона ожидается карта 0738.")]
    InvalidReference,
    #[error("Набор и размеры калибровок карты отличаются от эталона.")]
    IncompatibleMap,
}

/// One named bit in a CTM bitfield group.
#[derive(Debug, PartialEq, Eq)]
pub struct Flag {
    pub address: u16,
    pub bit: u8,
    pub title: String,
    pub section: String,
}

#[derive(Debug)]
pub struct Entry {
    pub path: Vec<String>,
    pub title: Option<String>,
    pub kind: Kind,
    pub id: [u8; 4],
    pub address: Option<u32>,
    pub shape: Vec<usize>,
    pub units: String,
    pub tags: Vec<String>,
    pub flags: Vec<Flag>,
    pub offset: usize,
    encoding: Option<Encoding>,
    axes: Vec<Option<[f64; 2]>>,
    axis_labels: Vec<String>,
}

impl Entry {
    pub fn numeric(&self, image: &Image) -> Result<MapCalibration, MappedError> {
        MapCalibration::resolve(image, self.address, &self.shape, self.encoding)
    }

    pub fn axis_label(&self, axis: usize) -> Option<&str> {
        self.axis_labels
            .get(axis)
            .map(String::as_str)
            .filter(|label| !label.is_empty())
    }

    pub fn axis_value(&self, image: &Image, axis: usize, index: usize) -> Option<f64> {
        let count = *self.shape.get(axis)?;
        if index >= count {
            return None;
        }
        let tag = self.tags.get(axis).map_or("", String::as_str);
        if let Some(scale) = tag
            .strip_prefix("!002-613C-")
            .and_then(|value| value.parse::<f64>().ok())
        {
            return image.rpm(index).map(|rpm| f64::from(rpm) * scale / 30.0);
        }
        if tag == "!004-6064-6066" {
            return image.load_axis(index).map(f64::from);
        }
        if tag.is_empty()
            && let Some(Some([start, end])) = self.axes.get(axis)
        {
            return Some(start + (end - start) * index as f64 / (count - 1).max(1) as f64);
        }
        None
    }

    pub fn calibration_kind(&self) -> Option<CalibrationKind> {
        match self.kind {
            Kind::Folder => None,
            Kind::Curve => Some(CalibrationKind::Table2D),
            Kind::Table => Some(CalibrationKind::Table3D),
            Kind::Scalar => Some(CalibrationKind::Number),
            Kind::Bitfield => Some(CalibrationKind::CheckboxList),
        }
    }

    pub fn parameter(&self) -> Option<Parameter> {
        Parameter::ALL.into_iter().find(|parameter| {
            let spec = parameter.spec();
            spec.id == self.id
                && Some(spec.address as u32) == self.address
                && spec.shape == self.shape
                && self.kind
                    == match spec.shape.len() {
                        0 => Kind::Scalar,
                        1 => Kind::Curve,
                        _ => Kind::Table,
                    }
        })
    }

    pub fn identifier(&self) -> String {
        self.id.iter().map(|byte| format!("{byte:02X}")).collect()
    }
}

pub struct Catalog {
    entries: Vec<Entry>,
    build: Build,
}

impl Catalog {
    /// Empty catalog for an image whose map has not been supplied yet.
    pub fn empty() -> Self {
        Self {
            entries: Vec::new(),
            build: Build::Ctp0738,
        }
    }

    /// Parses the record stream without consulting the filename or a firmware image.
    pub fn parse(data: &[u8]) -> Result<Self, MapError> {
        parse(data)
    }

    /// Checks IDs and shapes against a matching 0738 map and fills missing 0923 fields.
    ///
    /// Choose the reference for the firmware profile before calling this method.
    /// Addresses and other fields already present in the map are preserved.
    pub fn with_reference(mut self, reference: &Self) -> Result<Self, MapError> {
        if reference.build != Build::Ctp0738 {
            return Err(MapError::InvalidReference);
        }
        let index: HashMap<_, _> = self.entries.iter().map(|e| ((e.kind, e.id), e)).collect();
        let reference_index: HashMap<_, _> = reference
            .entries
            .iter()
            .map(|e| ((e.kind, e.id), e))
            .collect();
        if index.len() != self.entries.len()
            || reference_index.len() != reference.entries.len()
            || !reference_index.iter().all(|(key, entry)| {
                index.get(key).is_some_and(|other| {
                    entry.shape == other.shape
                        && (self.build == Build::Ctp0923 || other.address == entry.address)
                        && entry
                            .flags
                            .iter()
                            .map(|flag| (flag.address, flag.bit))
                            .eq(other.flags.iter().map(|flag| (flag.address, flag.bit)))
                })
            })
        {
            return Err(MapError::IncompatibleMap);
        }

        if self.build == Build::Ctp0923 {
            for entry in &mut self.entries {
                if let Some(original) = reference_index.get(&(entry.kind, entry.id)) {
                    entry.title.clone_from(&original.title);
                    entry.path.clone_from(&original.path);
                    entry.address = entry.address.or(original.address);
                    entry.encoding = original.encoding;
                    entry.axes.clone_from(&original.axes);
                    entry.axis_labels.clone_from(&original.axis_labels);
                    if entry.units.is_empty() {
                        entry.units.clone_from(&original.units);
                    }
                }
            }
        }
        Ok(self)
    }

    /// Identifies the file format, which does not establish which app last wrote it.
    pub fn build(&self) -> Build {
        self.build
    }

    pub fn entries(&self) -> &[Entry] {
        &self.entries
    }

    pub fn find(&self, parameter: Parameter) -> Option<usize> {
        self.entries
            .iter()
            .position(|entry| entry.parameter() == Some(parameter))
    }
}

fn parse(data: &[u8]) -> Result<Catalog, MapError> {
    let (build, mut offset, header, lengths) = match data.get(..6) {
        Some(b"CTM\x03\x01\x00") => (Build::Ctp0738, 0x2EA, 0x45, [0, 0x1AE, 0x2E2, 0x65, 0x1286]),
        Some(b"CTM\x04\x00\x00") => (Build::Ctp0923, 0x2E8, 0x4C, [0, 0x18F, 0x2C2, 0x4A, 0x123E]),
        _ => return Err(MapError::UnsupportedFormat),
    };
    let mut entries = Vec::new();
    let mut tree: Vec<(usize, String)> = Vec::new();
    while offset < data.len() {
        let error = || MapError::InvalidRecord { offset };
        let start = offset + header;
        let record = data.get(offset..start).ok_or_else(error)?;
        let depth = usize::from(record[header - 4]);
        let kind_index = usize::from(record[header - 3]);
        let length = word(record, header - 2);
        if !(1..=4).contains(&depth) || lengths.get(kind_index) != Some(&length) {
            return Err(error());
        }
        let kind = [
            Kind::Folder,
            Kind::Curve,
            Kind::Table,
            Kind::Scalar,
            Kind::Bitfield,
        ][kind_index];
        let body = data.get(start..start + length).ok_or_else(error)?;
        let name = if build == Build::Ctp0738 {
            field(&record[..0x41]).ok_or_else(error)?
        } else {
            String::new()
        };
        tree.retain(|(level, _)| *level < depth);
        if kind == Kind::Folder {
            tree.push((depth, name));
        } else {
            let id: [u8; 4] = body[..4].try_into().expect("record id");
            let address = if build == Build::Ctp0738 || kind == Kind::Bitfield {
                let mut raw = u32::from_le_bytes(body[4..8].try_into().expect("record address"));
                if build == Build::Ctp0738 {
                    raw ^= 0xD366019C;
                }
                Some(raw)
            } else {
                None
            };
            let mut entry = Entry {
                path: tree
                    .iter()
                    .map(|(_, name)| name.clone())
                    .filter(|name| !name.is_empty())
                    .collect(),
                title: (!name.is_empty()).then_some(name),
                kind,
                id,
                address,
                shape: vec![],
                units: String::new(),
                tags: vec![],
                flags: vec![],
                offset,
                axes: Vec::new(),
                axis_labels: Vec::new(),
                encoding: (build == Build::Ctp0738)
                    .then(|| numeric_encoding(kind, body))
                    .flatten(),
            };
            let tag = |position| field(&body[position..position + 0x100]).ok_or_else(error);
            match (build, kind) {
                (Build::Ctp0738, Kind::Curve) => {
                    entry.shape = vec![word(body, 0x152)];
                    entry.axes = vec![axis_limits(body, 0x156)];
                    entry.axis_labels = vec![field(&body[0x10..0x31]).ok_or_else(error)?];
                    entry.units = field(&body[0x31..0x52]).ok_or_else(error)?;
                    entry.tags = vec![tag(0x52)?];
                }
                (Build::Ctp0923, Kind::Curve) => {
                    entry.shape = vec![word(body, 0x1E)];
                    entry.tags = vec![tag(0x8D)?];
                }
                (Build::Ctp0738, Kind::Table) => {
                    entry.shape = vec![word(body, 0x273), word(body, 0x277)];
                    entry.axes = vec![axis_limits(body, 0x279), axis_limits(body, 0x28A)];
                    entry.axis_labels = vec![
                        field(&body[0x10..0x31]).ok_or_else(error)?,
                        field(&body[0x52..0x73]).ok_or_else(error)?,
                    ];
                    entry.units = field(&body[0x31..0x52]).ok_or_else(error)?;
                    entry.tags = vec![tag(0x73)?, tag(0x173)?];
                }
                (Build::Ctp0923, Kind::Table) => {
                    entry.shape = vec![usize::from(body[0x1E]), usize::from(body[0x20])];
                    entry.tags = vec![tag(0xC0)?, tag(0x1C0)?];
                }
                (Build::Ctp0738, Kind::Scalar) => {
                    entry.units = field(&body[0x10..0x21]).ok_or_else(error)?
                }
                (Build::Ctp0923, Kind::Scalar) => {
                    entry.units = field(&body[0x38..0x48]).ok_or_else(error)?
                }
                (_, Kind::Bitfield) => {
                    let width = if build == Build::Ctp0738 { 0x41 } else { 0x40 };
                    entry.flags = bitfield(body, width, entry.address).ok_or_else(error)?;
                }
                _ => {}
            }
            entries.push(entry);
        }
        offset = start + length;
    }
    if entries.is_empty() {
        return Err(MapError::Empty);
    }
    Ok(Catalog { entries, build })
}

fn axis_limits(body: &[u8], offset: usize) -> Option<[f64; 2]> {
    let read = |start| f64::from_le_bytes(body[start..start + 8].try_into().expect("CTM axis"));
    let (start, end) = (read(offset), read(offset + 8));
    (start.is_finite() && end.is_finite() && start < end).then_some([start, end])
}

fn numeric_encoding(kind: Kind, body: &[u8]) -> Option<Encoding> {
    let (representation, mode, divisor, multiplier, zero) = match kind {
        Kind::Scalar => (body[0x31], body[0x32], 0x33, 0x4B, 0x53),
        Kind::Curve => (u8::from(body[0x17B] == 1), body[0x17B], 0x17C, 0x194, 0x19C),
        Kind::Table => (u8::from(body[0x2AE] == 1), body[0x2AE], 0x2B0, 0x2C8, 0x2D0),
        _ => return None,
    };
    if kind == Kind::Table && mode == 0 {
        let mask = u8::try_from(word(body, 0x275)).ok()?;
        if mask == 0 {
            return None;
        }
        let bits = mask >> mask.trailing_zeros();
        if bits & bits.wrapping_add(1) != 0 {
            return None;
        }
        return Some(Encoding {
            storage: Storage::Bits(mask),
            scale: 1.0,
            zero: 0.0,
            physical: true,
        });
    }
    let storage = match representation {
        0 => Storage::U8,
        1 => Storage::I8,
        2 if kind == Kind::Scalar => Storage::U16Le,
        _ => return None,
    };
    let floating = |i| f64::from_le_bytes(body[i..i + 8].try_into().expect("CTM coefficient"));
    let scale = floating(multiplier) / floating(divisor);
    let zero = floating(zero);
    let physical = matches!((kind, mode), (_, 0xFF) | (Kind::Curve | Kind::Table, 1))
        && scale.is_finite()
        && scale != 0.0
        && zero.is_finite();
    Some(if physical {
        Encoding {
            storage,
            scale,
            zero,
            physical,
        }
    } else {
        Encoding {
            storage,
            scale: 1.0,
            zero: 0.0,
            physical,
        }
    })
}

fn bitfield(body: &[u8], width: usize, first_address: Option<u32>) -> Option<Vec<Flag>> {
    const BANKS: usize = 8;
    let label = |slot| {
        let start = 0x2C + slot * width;
        field(&body[start..start + width])
    };
    let mut flags = Vec::new();
    for bank in 0..BANKS {
        let offset = 4 + bank * 4;
        let address = if bank == 0 {
            first_address?
        } else {
            u32::from_le_bytes(body[offset..offset + 4].try_into().ok()?)
        };
        if address == u32::MAX {
            continue;
        }
        let address = u16::try_from(address).ok()?;
        let section = label(bank)?;
        for bit in 0..8 {
            let title = label(BANKS + bank * 8 + bit)?;
            if !matches!(title.as_str(), "" | "Резерв" | "Не используется") {
                flags.push(Flag {
                    address,
                    bit: bit as u8,
                    title,
                    section: section.clone(),
                });
            }
        }
    }
    Some(flags)
}

fn word(bytes: &[u8], offset: usize) -> usize {
    usize::from(u16::from_le_bytes([bytes[offset], bytes[offset + 1]]))
}

fn field(bytes: &[u8]) -> Option<String> {
    let length = usize::from(bytes[0]);
    let data = bytes.get(1..=length)?;
    if bytes[length + 1..].iter().any(|&byte| byte != 0) {
        return None;
    }
    let (value, _, _) = WINDOWS_1251.decode(data);
    Some(value.into_owned())
}

#[cfg(test)]
mod tests;
