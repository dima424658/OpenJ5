//! Offline preparation of one-bank images for the CombiLoader workflow.

use std::fs;
use std::path::{Path, PathBuf};

use sha2::{Digest, Sha256};

mod relay;
use thiserror::Error;

use crate::{FlashImageError, WriteImage, save_dump};

#[derive(Debug, Default)]
pub struct FlashOptions {
    pub original: bool,
    pub remove_calibration_checks: bool,
    pub remove_error_mask: bool,
    pub checksum: Option<u16>,
    pub starter_relay: Option<PathBuf>,
}

#[derive(Debug, Error)]
pub enum PrepareError {
    #[error(transparent)]
    Flash(#[from] FlashImageError),
    #[error("could not read the starter relay template: {0}")]
    RelayRead(#[from] std::io::Error),
    #[error("unexpected starter relay template size or SHA-256")]
    RelayTemplate,
    #[error("firmware {id} has no recognized {feature} patch sites")]
    Unavailable { id: String, feature: &'static str },
    #[error("no unused 256-byte checksum area in the Flash tail")]
    ChecksumArea,
    #[error("checksum compensation did not produce {0:04X}")]
    ChecksumMismatch(u16),
}

impl FlashOptions {
    /// Returns a checked write image. Original mode selects the input buffer
    /// before applying any option, matching the two original-buffer controls.
    pub fn prepare(self, bytes: Vec<u8>) -> Result<WriteImage, PrepareError> {
        if self.original || !self.requested() {
            return Ok(WriteImage::new(bytes)?);
        }

        // Check the profile and stored checksums before making code changes.
        let original = WriteImage::new(bytes)?;
        let mut bytes = original.bytes().to_vec();
        if self.remove_calibration_checks && !remove_calibration_checks(&mut bytes) {
            return Err(unavailable(&bytes, "calibration checksum"));
        }
        if self.remove_error_mask && !remove_error_mask(&mut bytes) {
            return Err(unavailable(&bytes, "additional error mask"));
        }

        if let Some(path) = self.starter_relay {
            let template = fs::read(path)?;
            relay::apply(&mut bytes, &template)?;
        }
        update_checksum(&mut bytes);
        if let Some(target) = self.checksum {
            fix_checksum(&mut bytes, target)?;
        }
        Ok(WriteImage::new(bytes)?)
    }
    /// Saves the prepared BIN and returns its checksum. Original mode saves a raw copy.
    pub fn save_prepared(self, bytes: Vec<u8>, output: &Path) -> Result<Option<u16>, PrepareError> {
        if self.original {
            save_dump(output, &bytes)?;
            return Ok(None);
        }
        let image = self.prepare(bytes)?;
        save_dump(output, image.bytes())?;
        Ok(Some(stored_checksum(image.bytes())))
    }

    pub fn requested(&self) -> bool {
        self.original
            || self.remove_calibration_checks
            || self.remove_error_mask
            || self.checksum.is_some()
            || self.starter_relay.is_some()
    }
}

fn unavailable(bytes: &[u8], feature: &'static str) -> PrepareError {
    let id = String::from_utf8_lossy(&bytes[0x6020..0x6028]).into_owned();
    PrepareError::Unavailable { id, feature }
}

// CombiLoader 2.18, 0x429798. Locate a checksum loop before examining
// up to 0x80 bytes for the ROM-fault branch.
fn remove_calibration_checks(bytes: &mut [u8]) -> bool {
    const CHECKSUM_LOOP: &[u8] =
        b"\x75\xf0\x08\xe4\x93\x28\xf8\xe4\xe9\x34\x00\xf9\xa3\xd5\xf0\xf3";
    const FULL_CHECKSUM_READ: &[u8] = b"\x90\xff\xfe\xe4\x93\xfa\x90\xff\xff\xe4\x93\xfb";
    let mut applied = false;
    for start in 0..0x6000 {
        if !bytes[start..].starts_with(CHECKSUM_LOOP)
            || (start..start + 0x80).any(|offset| bytes[offset..].starts_with(FULL_CHECKSUM_READ))
        {
            continue;
        }
        if let Some(branch) = (start..start + 0x80)
            .find(|&offset| bytes[offset] == 0x60 && bytes[offset + 2..offset + 4] == [0xd2, 0x17])
        {
            bytes[branch + 2..branch + 4].fill(0);
            applied = true;
        }
    }
    applied
}

// CombiLoader 2.18, 0x4298c0. The match must be unique within the code area.
fn remove_error_mask(bytes: &mut [u8]) -> bool {
    let mut positions = (0..0x6000).filter(|&offset| {
        bytes[offset..offset + 3] == [0xe4, 0x93, 0x44]
            && bytes[offset + 4..offset + 6] == [0x52, 0x5b]
    });
    let Some(offset) = positions.next() else {
        return false;
    };
    if positions.next().is_some() {
        return false;
    }
    bytes[offset + 2..offset + 4].fill(0);
    true
}

fn fix_checksum(bytes: &mut [u8], target: u16) -> Result<(), PrepareError> {
    let mut delta = target.wrapping_sub(stored_checksum(bytes));
    if delta == 0 {
        return Ok(());
    }
    // The original searches an aligned 256-byte window in an availability
    // map. For our supported profiles, only use zero padding in the tail.
    let length = 256.max(usize::from(delta).div_ceil(0xfe));
    let last = (0xfff0 - length) & !0x0f;
    let start = (0xf400..=last)
        .rev()
        .step_by(16)
        .find(|&offset| bytes[offset..offset + length].iter().all(|&byte| byte == 0))
        .ok_or(PrepareError::ChecksumArea)?;

    for byte in &mut bytes[start..start + length] {
        if delta == 0 {
            break;
        }
        let increment = delta.min(0xfe);
        *byte = increment as u8;
        delta -= increment;
    }
    update_checksum(bytes);
    if stored_checksum(bytes) != target {
        return Err(PrepareError::ChecksumMismatch(target));
    }
    Ok(())
}

pub fn parse_checksum(text: &str) -> Result<u16, std::num::ParseIntError> {
    let hex = text
        .strip_prefix("0x")
        .or_else(|| text.strip_prefix("0X"))
        .unwrap_or(text);
    u16::from_str_radix(hex, 16)
}

fn stored_checksum(bytes: &[u8]) -> u16 {
    u16::from_le_bytes([bytes[0xfffe], bytes[0xffff]])
}

fn update_checksum(bytes: &mut [u8]) {
    janvar_editor::firmware::update_flash_checksum(bytes.try_into().expect("validated Flash size"));
}

#[cfg(test)]
mod tests {
    use super::*;

    const L19: &[u8] = include_bytes!("../../../bins/J5v05l19.bin");

    #[test]
    fn preparation_matches_combi_loader_patch_sites() {
        let image = FlashOptions {
            remove_calibration_checks: true,
            remove_error_mask: true,
            ..FlashOptions::default()
        }
        .prepare(L19.to_vec())
        .unwrap();

        let changes: Vec<_> = L19
            .iter()
            .zip(image.bytes())
            .enumerate()
            .filter_map(|(offset, (before, after))| {
                (offset < 0xfffa && before != after).then_some((offset, *after))
            })
            .collect();
        assert_eq!(
            changes,
            [
                (0x4d16, 0),
                (0x4d17, 0),
                (0x532a, 0),
                (0x532b, 0),
                (0x53ab, 0),
                (0x53ac, 0)
            ]
        );
    }

    #[test]
    fn fixed_checksum_matches_the_whole_image_even_across_wrap() {
        let initial = stored_checksum(L19);
        for target in [0, 0xffff, initial, initial.wrapping_sub(1)] {
            let image = FlashOptions {
                checksum: Some(target),
                ..FlashOptions::default()
            }
            .prepare(L19.to_vec())
            .unwrap();
            let measured = image
                .bytes()
                .iter()
                .fold(0u16, |sum, &byte| sum.wrapping_add(u16::from(byte)));
            assert_eq!(measured, target);
            assert_eq!(stored_checksum(image.bytes()), target);
            assert_eq!(&image.bytes()[0xfffa..0xfffc], &(!target).to_le_bytes());
        }
    }

    #[test]
    fn original_mode_does_not_apply_the_other_options() {
        let image = FlashOptions {
            original: true,
            remove_calibration_checks: true,
            remove_error_mask: true,
            checksum: Some(0x1234),
            starter_relay: Some("unused".into()),
        }
        .prepare(L19.to_vec())
        .unwrap();
        assert_eq!(image.bytes(), L19);
    }
}
