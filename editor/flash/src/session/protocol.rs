//! Command framing and reply validation after the C509 loader starts.

use std::io;
use std::time::Duration;

use crate::{BLOCK_SIZE, frame_sum};

const MAX_REQUEST_SIZE: usize = BLOCK_SIZE + 7;
const MAX_REPLY_SIZE: usize = 0x90;

#[derive(Clone, Copy)]
pub(super) enum Command {
    FlashProbe = 0x11,
    EraseFlash = 0x12,
    ControllerFlags = 0x13,
    ReadFlash = 0x14,
    WriteFlash = 0x15,
    ReadEeprom = 0x16,
    WriteEeprom = 0x17,
    FlashLayout = 0x19,
    FinalizeFlash = 0x1a,
}

impl Command {
    pub(super) fn timeout(self) -> Duration {
        // Read constants from the CombiLoader command handlers, in milliseconds.
        Duration::from_millis(match self {
            Self::EraseFlash | Self::FinalizeFlash => 8_000,
            Self::WriteEeprom => 2_500,
            Self::ReadEeprom => 1_000,
            Self::WriteFlash => 500,
            Self::FlashProbe | Self::ControllerFlags | Self::ReadFlash | Self::FlashLayout => 2_000,
        })
    }

    pub(super) fn attempts(self) -> usize {
        match self {
            Self::FlashProbe
            | Self::ControllerFlags
            | Self::ReadFlash
            | Self::ReadEeprom
            | Self::FlashLayout => 4,
            // A missing acknowledgement does not establish whether the ECU
            // executed the operation. Keep the original failure visible.
            _ => 1,
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub(super) enum Echo {
    Required,
    Optional,
}

pub(super) struct Request {
    bytes: [u8; MAX_REQUEST_SIZE],
    len: usize,
}

impl Request {
    pub(super) fn new(command: Command, parameters: &[u8], data: &[u8]) -> Self {
        let len = parameters.len() + data.len() + 3;
        let mut bytes = [0; MAX_REQUEST_SIZE];
        bytes[..2].copy_from_slice(&[len as u8, command as u8]);
        bytes[2..2 + parameters.len()].copy_from_slice(parameters);
        bytes[2 + parameters.len()..len - 1].copy_from_slice(data);
        bytes[len - 1] = frame_sum(&bytes[..len - 1]);
        Self { bytes, len }
    }

    pub(super) fn bytes(&self) -> &[u8] {
        &self.bytes[..self.len]
    }
}

pub(super) struct Reply {
    bytes: [u8; MAX_REPLY_SIZE],
    len: usize,
}

impl Reply {
    pub(super) fn new(len: u8) -> io::Result<Self> {
        let len = usize::from(len);
        if !(3..=MAX_REPLY_SIZE).contains(&len) {
            return Err(invalid(format!("invalid ECU frame length {len:#04x}")));
        }
        let mut bytes = [0; MAX_REPLY_SIZE];
        bytes[0] = len as u8;
        Ok(Self { bytes, len })
    }

    pub(super) fn remaining(&mut self) -> &mut [u8] {
        &mut self.bytes[1..self.len]
    }

    pub(super) fn bytes(&self) -> &[u8] {
        &self.bytes[..self.len]
    }
}

pub(super) fn payload<const N: usize>(command: Command, frame: &[u8]) -> io::Result<[u8; N]> {
    let expected_code = command as u8 | 0x80;
    if frame[1] != expected_code {
        return Err(invalid(format!(
            "expected ECU response {expected_code:02X}, received {:02X}",
            frame[1]
        )));
    }
    let Some((&checksum, body)) = frame.split_last() else {
        unreachable!("Reply validates the minimum frame length");
    };
    if checksum != frame_sum(body) {
        return Err(invalid(format!(
            "invalid checksum for ECU response {expected_code:02X}"
        )));
    }
    if N > 1 && body[2..] == [0xaa] {
        return Err(invalid(format!(
            "ECU response {expected_code:02X} reported status AA"
        )));
    }
    body[2..].try_into().map_err(|_| {
        invalid(format!(
            "ECU response {expected_code:02X} has {} data bytes, expected {N}",
            body.len() - 2
        ))
    })
}

fn invalid(message: String) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message)
}
