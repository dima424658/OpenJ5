//! Flash block frames exchanged after the ECU loader has started.

use thiserror::Error;

pub use crate::BLOCK_SIZE;
use crate::{block_address, frame_sum};

pub const RESPONSE_SIZE: usize = BLOCK_SIZE + 3;

#[derive(Debug, Error, PartialEq, Eq)]
pub enum ReadFrameError {
    #[error("invalid Flash block offset {0:#06x}")]
    Offset(usize),
    #[error("expected {RESPONSE_SIZE} response bytes, received {0}")]
    Length(usize),
    #[error("unexpected Flash read response header {0:02x} {1:02x}")]
    Header(u8, u8),
    #[error("Flash read response checksum is {actual:#04x}, expected {expected:#04x}")]
    Checksum { actual: u8, expected: u8 },
}

/// Encodes a read request for a 128-byte block in the 64 KiB image.
pub fn request(offset: usize) -> Result<[u8; 7], ReadFrameError> {
    let [high, low] = block_address(offset).ok_or(ReadFrameError::Offset(offset))?;
    let mut frame = [7, 0x14, 0, high, low, BLOCK_SIZE as u8, 0];
    frame[6] = frame_sum(&frame[..6]);
    Ok(frame)
}

/// Checks the complete response and returns its Flash bytes.
pub fn payload(frame: &[u8]) -> Result<&[u8; BLOCK_SIZE], ReadFrameError> {
    let frame: &[u8; RESPONSE_SIZE] = frame
        .try_into()
        .map_err(|_| ReadFrameError::Length(frame.len()))?;
    if frame[..2] != [RESPONSE_SIZE as u8, 0x94] {
        return Err(ReadFrameError::Header(frame[0], frame[1]));
    }
    let actual = frame[RESPONSE_SIZE - 1];
    let expected = frame_sum(&frame[..RESPONSE_SIZE - 1]);
    if actual != expected {
        return Err(ReadFrameError::Checksum { actual, expected });
    }
    let [_, _, data @ .., _] = frame;
    Ok(data)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn encodes_first_and_last_captured_requests() {
        assert_eq!(request(0).unwrap(), [7, 0x14, 0, 0, 0, 0x80, 0x9b]);
        assert_eq!(
            request(0xff80).unwrap(),
            [7, 0x14, 0, 0xff, 0x80, 0x80, 0x1a]
        );
        assert_eq!(request(1), Err(ReadFrameError::Offset(1)));
        assert_eq!(request(0x10000), Err(ReadFrameError::Offset(0x10000)));
    }

    #[test]
    fn rejects_incomplete_misdirected_and_corrupt_responses() {
        let mut frame = [0; RESPONSE_SIZE];
        frame[..2].copy_from_slice(&[0x83, 0x94]);
        frame[2] = 0x02;
        frame[RESPONSE_SIZE - 1] = 0x19;
        assert_eq!(payload(&frame).unwrap()[0], 0x02);
        assert_eq!(payload(&frame[..2]), Err(ReadFrameError::Length(2)));

        frame[2] = 0x01;
        assert_eq!(
            payload(&frame),
            Err(ReadFrameError::Checksum {
                actual: 0x19,
                expected: 0x18
            })
        );

        frame[1] = 0x95;
        assert_eq!(payload(&frame), Err(ReadFrameError::Header(0x83, 0x95)));
    }
}
