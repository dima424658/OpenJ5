//! Image validation and Flash frames for the Janvar 5.1 loader.

pub mod advanced;
pub mod programmer;
pub mod read;
pub mod session;
pub mod write;

use std::io::{self, Write};
use std::path::Path;

use janvar_editor::firmware::{ChecksumKind, FirmwareError, IMAGE_SIZE, Image, Profile};
use thiserror::Error;

pub const BLOCK_SIZE: usize = 128;
pub const EEPROM_SIZE: usize = 512;

#[derive(Debug, Error)]
pub enum FlashImageError {
    #[error(transparent)]
    Firmware(#[from] FirmwareError),
    #[error(
        "expected a supported J5V05J16, J5V05L19 or J5V05N35 image for 2112-1411020-41, got {0}"
    )]
    WrongProfile(String),
    #[error("{kind:?} checksum is {stored:#06x}, expected {calculated:#06x}")]
    Checksum {
        kind: ChecksumKind,
        stored: u16,
        calculated: u16,
    },
    #[error("expected {IMAGE_SIZE} Flash bytes, received {0}")]
    ReadbackSize(usize),
    #[error("Flash differs at offset {offset:#06x}: expected {expected:#04x}, read {actual:#04x}")]
    ReadbackMismatch {
        offset: usize,
        expected: u8,
        actual: u8,
    },
    #[error("could not save the Flash dump: {0}")]
    Save(#[source] io::Error),
}

/// A supported BIN whose stored checksums match its contents.
pub struct WriteImage(Image);

impl WriteImage {
    pub fn new(bytes: Vec<u8>) -> Result<Self, FlashImageError> {
        let image = Image::new(bytes)?;
        let supported = match image.software_id() {
            "J5V05J16" => image.bytes().starts_with(b"\x02\x13\x58\x02\x07\x45"),
            "J5V05N35" => image.bytes().starts_with(b"\x02\x13\x52\x02\x07\x45"),
            _ => image.profile() == Profile::L19,
        };
        if !supported {
            return Err(FlashImageError::WrongProfile(
                image.software_id().to_owned(),
            ));
        }
        if let Some(checksum) = image.checksums().iter().find(|checksum| !checksum.valid()) {
            return Err(FlashImageError::Checksum {
                kind: checksum.kind,
                stored: checksum.stored,
                calculated: checksum.calculated,
            });
        }
        Ok(Self(image))
    }

    pub fn bytes(&self) -> &[u8] {
        self.0.bytes()
    }

    pub fn verify_readback(&self, readback: &[u8]) -> Result<(), FlashImageError> {
        check_dump_size(readback)?;
        for (offset, (&expected, &actual)) in self.bytes().iter().zip(readback).enumerate() {
            if expected != actual {
                return Err(FlashImageError::ReadbackMismatch {
                    offset,
                    expected,
                    actual,
                });
            }
        }
        Ok(())
    }
}

/// Saves a full dump byte for byte, with no replacement of an existing backup.
pub fn save_dump(path: &Path, bytes: &[u8]) -> Result<(), FlashImageError> {
    check_dump_size(bytes)?;
    save_bytes(path, bytes).map_err(FlashImageError::Save)
}

pub fn save_eeprom(path: &Path, bytes: &[u8; EEPROM_SIZE]) -> io::Result<()> {
    save_bytes(path, bytes)
}

fn save_bytes(path: &Path, bytes: &[u8]) -> io::Result<()> {
    let directory = path
        .parent()
        .filter(|path| !path.as_os_str().is_empty())
        .unwrap_or_else(|| Path::new("."));
    let mut temporary = tempfile::NamedTempFile::new_in(directory)?;
    temporary.write_all(bytes)?;
    temporary.as_file().sync_all()?;
    temporary
        .persist_noclobber(path)
        .map_err(|error| error.error)?;
    Ok(())
}

fn check_dump_size(bytes: &[u8]) -> Result<(), FlashImageError> {
    if bytes.len() != IMAGE_SIZE {
        return Err(FlashImageError::ReadbackSize(bytes.len()));
    }
    Ok(())
}

fn block_address(offset: usize) -> Option<[u8; 2]> {
    if offset >= IMAGE_SIZE || !offset.is_multiple_of(BLOCK_SIZE) {
        return None;
    }
    Some((offset as u16).to_be_bytes())
}

fn frame_sum(bytes: &[u8]) -> u8 {
    bytes.iter().copied().fold(0, u8::wrapping_add)
}

#[cfg(test)]
mod tests {
    use super::*;

    const L19: &[u8] = include_bytes!("../../../bins/J5v05l19.bin");
    const I02: &[u8] = include_bytes!("../../../bins/J5v13i02.bin");

    #[test]
    fn validates_the_stock_l19_image_and_detects_readback_errors() {
        let image = WriteImage::new(L19.to_vec()).unwrap();
        assert_eq!(image.bytes(), L19);
        image.verify_readback(L19).unwrap();
        assert!(matches!(
            image.verify_readback(&L19[..32]),
            Err(FlashImageError::ReadbackSize(32))
        ));
        let mut different = L19.to_vec();
        different[0x7000] ^= 1;
        assert!(matches!(
            image.verify_readback(&different),
            Err(FlashImageError::ReadbackMismatch { offset: 0x7000, .. })
        ));
        assert!(matches!(
            WriteImage::new(different),
            Err(FlashImageError::Checksum { .. })
        ));
        assert!(matches!(
            WriteImage::new(I02.to_vec()),
            Err(FlashImageError::WrongProfile(id)) if id == "J5V13I02"
        ));
    }

    #[test]
    fn only_publishes_complete_dumps_and_preserves_existing_backups() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("flash.bin");
        assert!(matches!(
            save_dump(&path, &L19[..32]),
            Err(FlashImageError::ReadbackSize(32))
        ));
        assert!(!path.exists());
        save_dump(&path, L19).unwrap();
        assert!(matches!(
            save_dump(&path, I02),
            Err(FlashImageError::Save(error)) if error.kind() == io::ErrorKind::AlreadyExists
        ));
        assert_eq!(std::fs::read(path).unwrap(), L19);
    }
}
