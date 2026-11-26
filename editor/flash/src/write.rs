//! Flash block writes and acknowledgements in the programming session.

use thiserror::Error;

use crate::{BLOCK_SIZE, block_address, frame_sum};

pub const REQUEST_SIZE: usize = BLOCK_SIZE + 7;
pub const ACK_SIZE: usize = 4;
pub const RESPONSE_SIZE: usize = REQUEST_SIZE + ACK_SIZE;
const ACK: [u8; ACK_SIZE] = [0x04, 0x95, 0x55, 0xee];

#[derive(Debug, Error, PartialEq, Eq)]
pub enum WriteFrameError {
    #[error("invalid Flash block offset {0:#06x}")]
    Offset(usize),
    #[error("expected {RESPONSE_SIZE} response bytes including echo, received {0}")]
    Length(usize),
    #[error("write request echo differs from the sent frame")]
    Echo,
    #[error("unexpected Flash write acknowledgement {0:02x?}")]
    Acknowledgement([u8; ACK_SIZE]),
}

pub fn request(
    offset: usize,
    data: &[u8; BLOCK_SIZE],
) -> Result<[u8; REQUEST_SIZE], WriteFrameError> {
    let [high, low] = block_address(offset).ok_or(WriteFrameError::Offset(offset))?;
    let mut frame = [0; REQUEST_SIZE];
    frame[..6].copy_from_slice(&[REQUEST_SIZE as u8, 0x15, 0, high, low, BLOCK_SIZE as u8]);
    frame[6..REQUEST_SIZE - 1].copy_from_slice(data);
    frame[REQUEST_SIZE - 1] = frame_sum(&frame[..REQUEST_SIZE - 1]);
    Ok(frame)
}

/// Verifies the echoed write request and the ECU's separate acknowledgement.
pub fn acknowledge(sent: &[u8; REQUEST_SIZE], received: &[u8]) -> Result<(), WriteFrameError> {
    let received: &[u8; RESPONSE_SIZE] = received
        .try_into()
        .map_err(|_| WriteFrameError::Length(received.len()))?;
    let [echo @ .., a, b, c, d] = received;
    if echo != sent {
        return Err(WriteFrameError::Echo);
    }
    let ack = [*a, *b, *c, *d];
    if ack != ACK {
        return Err(WriteFrameError::Acknowledgement(ack));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn encodes_a_block_with_address_and_checksum() {
        let data = [0x11; BLOCK_SIZE];
        let frame = request(0xff80, &data).unwrap();
        assert_eq!(frame[..6], [0x87, 0x15, 0, 0xff, 0x80, 0x80]);
        assert_eq!(frame[6..REQUEST_SIZE - 1], data);
        assert_eq!(frame[REQUEST_SIZE - 1], 0x1b);
        assert_eq!(request(1, &data), Err(WriteFrameError::Offset(1)));
    }

    #[test]
    fn requires_both_the_echo_and_the_success_acknowledgement() {
        let sent = request(0, &[0x00; BLOCK_SIZE]).unwrap();
        let mut received = [0; RESPONSE_SIZE];
        received[..REQUEST_SIZE].copy_from_slice(&sent);
        received[REQUEST_SIZE..].copy_from_slice(&ACK);
        assert_eq!(acknowledge(&sent, &received), Ok(()));
        assert_eq!(
            acknowledge(&sent, &received[..REQUEST_SIZE]),
            Err(WriteFrameError::Length(REQUEST_SIZE))
        );
        received[5] ^= 1;
        assert_eq!(acknowledge(&sent, &received), Err(WriteFrameError::Echo));
        received[5] ^= 1;
        received[RESPONSE_SIZE - 1] ^= 1;
        assert_eq!(
            acknowledge(&sent, &received),
            Err(WriteFrameError::Acknowledgement([4, 0x95, 0x55, 0xef]))
        );
    }
}
