//! The C509 bootstrap and the CombiLoader programming exchange.

use std::io::{self, Read, Write};
use std::thread;
use std::time::{Duration, Instant};

use janvar_editor::firmware::IMAGE_SIZE;
use sha2::{Digest, Sha256};

use crate::{BLOCK_SIZE, EEPROM_SIZE, WriteImage, frame_sum, read, write};

const BLOCK_TIMEOUT: Duration = Duration::from_secs(2);
const CONTROL_TIMEOUT: Duration = Duration::from_secs(10);
const PROBE_TIMEOUT: Duration = Duration::from_millis(500);
const PROBE_INTERVAL: Duration = Duration::from_millis(250);
const BOOT_FRAME_COUNT: usize = 145;
const BOOT_FRAME_SIZE: usize = 18;
const BOOT_DATA_END: usize = 9 + BOOT_FRAME_COUNT * BOOT_FRAME_SIZE;
const BOOT_FILE_SIZE: usize = BOOT_DATA_END + 8;
const BOOT_SHA256: [u8; 32] = [
    0xeb, 0x5c, 0x50, 0xf7, 0xe6, 0x56, 0x62, 0xda, 0x75, 0x62, 0x88, 0x4e, 0x0e, 0x9c, 0x61, 0xbd,
    0xc4, 0xe5, 0x00, 0xa4, 0x25, 0x7b, 0x09, 0x6f, 0xaf, 0x4d, 0x27, 0xff, 0xf6, 0x0a, 0x8c, 0xbc,
];

/// The bootstrap byte stream verified against the three successful captures.
pub struct Bootstrap<'a>(&'a [u8]);

impl<'a> Bootstrap<'a> {
    pub fn new(bytes: &'a [u8]) -> io::Result<Self> {
        if bytes.len() != BOOT_FILE_SIZE || Sha256::digest(bytes)[..] != BOOT_SHA256 {
            return Err(invalid("unexpected bootstrap file size or SHA-256"));
        }
        Ok(Self(bytes))
    }

    fn frames(&self) -> impl Iterator<Item = &[u8]> {
        [&self.0[1..9]]
            .into_iter()
            .chain(
                self.0[9..BOOT_DATA_END]
                    .as_chunks::<BOOT_FRAME_SIZE>()
                    .0
                    .iter()
                    .map(<[u8; BOOT_FRAME_SIZE]>::as_slice),
            )
            .chain([&self.0[BOOT_DATA_END..]])
    }
}

pub trait Link: Read + Write {
    fn set_read_timeout(&mut self, timeout: Duration) -> io::Result<()>;

    fn discard_input(&mut self) -> io::Result<()> {
        Ok(())
    }
}

#[cfg(windows)]
impl Link for serialport::COMPort {
    fn set_read_timeout(&mut self, timeout: Duration) -> io::Result<()> {
        serialport::SerialPort::set_timeout(self, timeout.max(Duration::from_millis(1)))
            .map_err(io::Error::other)
    }

    fn discard_input(&mut self) -> io::Result<()> {
        serialport::SerialPort::clear(self, serialport::ClearBuffer::Input)
            .map_err(io::Error::other)
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Stage {
    Bootstrap,
    ReadFlash,
    EraseFlash,
    WriteFlash,
    FinishFlash,
    ReadEeprom,
    WriteEeprom,
    ClearEeprom,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Progress {
    pub stage: Stage,
    pub completed: usize,
    pub total: usize,
}

/// An identified, active programming session on the captured hardware profile.
pub struct Session<T> {
    link: T,
    flash_probe: Option<[u8; 2]>,
    progress: Option<Box<dyn FnMut(Progress)>>,
}

impl<T: Link> Session<T> {
    pub fn start(link: T, bootstrap: &Bootstrap<'_>) -> io::Result<Self> {
        let mut session = Self {
            link,
            flash_probe: None,
            progress: None,
        };
        session
            .send_bootstrap_frame(&[0x00])
            .map_err(|error| at("bootstrap frame 0", error))?;
        session.finish_start(bootstrap)
    }

    pub fn start_waiting(
        link: T,
        bootstrap: &Bootstrap<'_>,
        timeout: Duration,
        progress: impl FnMut(Progress) + 'static,
    ) -> io::Result<Self> {
        let mut session = Self {
            link,
            flash_probe: None,
            progress: Some(Box::new(progress)),
        };
        session.wait_for_connection(timeout)?;
        session.finish_start(bootstrap)
    }

    fn finish_start(mut self, bootstrap: &Bootstrap<'_>) -> io::Result<Self> {
        let total = BOOT_FRAME_COUNT + 2;
        self.report(Stage::Bootstrap, 0, total);
        for (index, frame) in bootstrap.frames().enumerate() {
            let completed = index + 1;
            self.send_bootstrap_frame(frame)
                .map_err(|error| at(&format!("bootstrap frame {completed}"), error))?;
            self.report(Stage::Bootstrap, completed, total);
        }
        self.identify_flash()
            .map_err(|error| at("identify Flash", error))?;
        Ok(self)
    }

    fn identify_flash(&mut self) -> io::Result<()> {
        let flags = self.control_reply::<5>(0x13)?;
        match flags {
            [0x05, 0x93, 0x55, 0x04, 0xf1] => Ok(()),
            [0x05, 0x93, 0x55, 0x00, 0xed] => {
                let probe = self.control_reply::<5>(0x11)?;
                let [0x05, 0x91, first, second, checksum] = probe else {
                    return Err(invalid(format!(
                        "unexpected Flash probe response {probe:02x?}"
                    )));
                };
                if checksum != frame_sum(&probe[..4]) {
                    return Err(invalid("invalid Flash probe response checksum"));
                }
                self.flash_probe = Some([first, second]);
                Ok(())
            }
            _ => Err(invalid(format!(
                "unsupported Flash flag response {flags:02x?}"
            ))),
        }
    }

    /// Returns the raw probe bytes captured for Flash flag 00.
    pub fn flash_probe(&self) -> Option<[u8; 2]> {
        self.flash_probe
    }

    fn wait_for_connection(&mut self, timeout: Duration) -> io::Result<()> {
        let deadline = Instant::now().checked_add(timeout).ok_or_else(|| {
            io::Error::new(io::ErrorKind::InvalidInput, "invalid connection timeout")
        })?;
        loop {
            let now = Instant::now();
            if now >= deadline {
                return Err(io::Error::new(
                    io::ErrorKind::TimedOut,
                    format!(
                        "No ECU response within {} seconds. Check power and programming mode.",
                        timeout.as_secs_f64()
                    ),
                ));
            }
            let attempt_deadline = (now + PROBE_TIMEOUT).min(deadline);
            self.link.discard_input()?;
            let remaining = attempt_deadline.saturating_duration_since(Instant::now());
            if remaining.is_zero() {
                continue;
            }
            self.link.set_read_timeout(remaining)?;
            self.link.write_all(&[0x00])?;
            match self.expect(&[0x00, 0x55], attempt_deadline) {
                Ok(()) => return Ok(()),
                Err(error)
                    if matches!(
                        error.kind(),
                        io::ErrorKind::TimedOut
                            | io::ErrorKind::UnexpectedEof
                            | io::ErrorKind::InvalidData
                    ) =>
                {
                    let next_probe = (now + PROBE_INTERVAL).min(deadline);
                    thread::sleep(next_probe.saturating_duration_since(Instant::now()));
                }
                Err(error) => return Err(at("connect to ECU", error)),
            }
        }
    }

    fn report(&mut self, stage: Stage, completed: usize, total: usize) {
        if let Some(handler) = &mut self.progress {
            handler(Progress {
                stage,
                completed,
                total,
            });
        }
    }

    pub fn read_flash(&mut self) -> io::Result<Vec<u8>> {
        self.control(0x19, &[0x04, 0x99, 0x55, 0xf2])
            .map_err(|error| at("prepare Flash read", error))?;
        let mut image = Vec::with_capacity(IMAGE_SIZE);
        let total = IMAGE_SIZE / BLOCK_SIZE;
        self.report(Stage::ReadFlash, 0, total);
        for offset in (0..IMAGE_SIZE).step_by(BLOCK_SIZE) {
            let request = read::request(offset).map_err(io::Error::other)?;
            let mut response = [0; read::RESPONSE_SIZE];
            self.send(&request)
                .and_then(|()| {
                    let deadline = Instant::now() + BLOCK_TIMEOUT;
                    self.receive(&mut response[..1], deadline)?;
                    if response[0] == request[0] {
                        self.expect(&request[1..], deadline)?;
                        self.receive(&mut response, deadline)
                    } else {
                        self.receive(&mut response[1..], deadline)
                    }
                })
                .map_err(|error| block_at("read Flash", offset, error))?;
            let data = read::payload(&response)
                .map_err(|error| block_at("read Flash", offset, io::Error::other(error)))?;
            image.extend_from_slice(data);
            self.report(Stage::ReadFlash, image.len() / BLOCK_SIZE, total);
        }
        Ok(image)
    }

    pub fn clear_flash(&mut self) -> io::Result<()> {
        self.report(Stage::EraseFlash, 0, 1);
        // The flag 00 identification path already completed command 11.
        if self.flash_probe.is_none() {
            self.control(0x11, &[0x05, 0x91, 0x00, 0x80, 0x16])
                .map_err(|error| at("prepare Flash erase", error))?;
        }
        self.control(0x12, &[0x04, 0x92, 0x55, 0xeb])
            .map_err(|error| at("erase Flash", error))?;
        self.report(Stage::EraseFlash, 1, 1);
        Ok(())
    }

    pub fn write_flash(&mut self, image: &WriteImage) -> io::Result<()> {
        self.clear_flash()?;
        let (blocks, _) = image.bytes().as_chunks::<BLOCK_SIZE>();
        self.report(Stage::WriteFlash, 0, blocks.len());
        for (block, data) in blocks.iter().enumerate() {
            let offset = block * BLOCK_SIZE;
            let request = write::request(offset, data).map_err(io::Error::other)?;
            let mut response = [0; write::RESPONSE_SIZE];
            self.send(&request)
                .and_then(|()| self.receive(&mut response, Instant::now() + BLOCK_TIMEOUT))
                .map_err(|error| block_at("write Flash", offset, error))?;
            write::acknowledge(&request, &response)
                .map_err(|error| block_at("write Flash", offset, io::Error::other(error)))?;
            self.report(Stage::WriteFlash, block + 1, blocks.len());
        }
        self.report(Stage::FinishFlash, 0, 1);
        self.control(0x1a, &[0x04, 0x9a, 0x55, 0xf3])
            .map_err(|error| at("finish Flash write", error))?;
        self.report(Stage::FinishFlash, 1, 1);
        Ok(())
    }

    pub fn read_eeprom(&mut self) -> io::Result<[u8; EEPROM_SIZE]> {
        let mut image = [0; EEPROM_SIZE];
        let (blocks, _) = image.as_chunks_mut::<BLOCK_SIZE>();
        let total = blocks.len();
        self.report(Stage::ReadEeprom, 0, total);
        for (block, data) in blocks.iter_mut().enumerate() {
            let offset = block * BLOCK_SIZE;
            let [high, low] = (offset as u16).to_be_bytes();
            let mut request = [0x06, 0x16, high, low, BLOCK_SIZE as u8, 0];
            request[5] = frame_sum(&request[..5]);
            self.read_eeprom_block(&request, data)
                .map_err(|error| block_at("read EEPROM", offset, error))?;
            self.report(Stage::ReadEeprom, block + 1, total);
        }
        Ok(image)
    }

    pub fn write_eeprom(&mut self, image: &[u8; EEPROM_SIZE]) -> io::Result<()> {
        self.transfer_eeprom(image, Stage::WriteEeprom)
    }

    pub fn clear_eeprom(&mut self) -> io::Result<()> {
        self.transfer_eeprom(&[0xff; EEPROM_SIZE], Stage::ClearEeprom)
            .map_err(|error| at("clear EEPROM", error))
    }

    fn transfer_eeprom(&mut self, image: &[u8; EEPROM_SIZE], stage: Stage) -> io::Result<()> {
        let (blocks, _) = image.as_chunks::<BLOCK_SIZE>();
        self.report(stage, 0, blocks.len());
        for (block, data) in blocks.iter().enumerate() {
            let offset = block * BLOCK_SIZE;
            let [high, low] = (offset as u16).to_be_bytes();
            let mut request = [0; BLOCK_SIZE + 6];
            request[..5].copy_from_slice(&[0x86, 0x17, high, low, BLOCK_SIZE as u8]);
            request[5..5 + BLOCK_SIZE].copy_from_slice(data);
            request[5 + BLOCK_SIZE] = frame_sum(&request[..5 + BLOCK_SIZE]);
            self.echoed(&request, &[0x04, 0x97, 0x55, 0xf0], BLOCK_TIMEOUT)
                .map_err(|error| block_at("write EEPROM", offset, error))?;
            self.report(stage, block + 1, blocks.len());
        }
        Ok(())
    }

    fn read_eeprom_block(&mut self, request: &[u8], data: &mut [u8]) -> io::Result<()> {
        self.send(request)?;
        let deadline = Instant::now() + BLOCK_TIMEOUT;
        self.expect(request, deadline)?;
        let mut frame = [0; BLOCK_SIZE + 4];
        self.receive(&mut frame, deadline)?;
        if frame[..3] != [0x84, 0x96, 0x55]
            || frame_sum(&frame[..frame.len() - 1]) != frame[frame.len() - 1]
        {
            return Err(invalid("unexpected EEPROM response header or checksum"));
        }
        data.copy_from_slice(&frame[3..frame.len() - 1]);
        Ok(())
    }

    fn send_bootstrap_frame(&mut self, frame: &[u8]) -> io::Result<()> {
        for &byte in frame {
            self.send(&[byte])?;
            self.expect(&[byte], Instant::now() + BLOCK_TIMEOUT)?;
        }
        self.expect(&[0x55], Instant::now() + BLOCK_TIMEOUT)
    }

    fn control(&mut self, command: u8, response: &[u8]) -> io::Result<()> {
        self.echoed(&control_request(command), response, CONTROL_TIMEOUT)
    }

    fn control_reply<const N: usize>(&mut self, command: u8) -> io::Result<[u8; N]> {
        let request = control_request(command);
        self.send(&request)?;
        let deadline = Instant::now() + CONTROL_TIMEOUT;
        self.expect(&request, deadline)?;
        let mut response = [0; N];
        self.receive(&mut response, deadline)?;
        Ok(response)
    }

    fn echoed(&mut self, request: &[u8], response: &[u8], timeout: Duration) -> io::Result<()> {
        self.send(request)?;
        let deadline = Instant::now() + timeout;
        self.expect(request, deadline)?;
        self.expect(response, deadline)
    }

    fn send(&mut self, bytes: &[u8]) -> io::Result<()> {
        self.link.set_read_timeout(BLOCK_TIMEOUT)?;
        self.link.write_all(bytes)
    }

    fn expect(&mut self, expected: &[u8], deadline: Instant) -> io::Result<()> {
        let mut received = [0; write::REQUEST_SIZE];
        let received = &mut received[..expected.len()];
        self.receive(received, deadline)?;
        if received != expected {
            return Err(invalid(format!(
                "expected {expected:02x?}, received {received:02x?}"
            )));
        }
        Ok(())
    }

    fn receive(&mut self, mut bytes: &mut [u8], deadline: Instant) -> io::Result<()> {
        while !bytes.is_empty() {
            let remaining = deadline.saturating_duration_since(Instant::now());
            if remaining.is_zero() {
                return Err(io::Error::new(
                    io::ErrorKind::TimedOut,
                    "no complete response",
                ));
            }
            self.link.set_read_timeout(remaining)?;
            match self.link.read(bytes) {
                Ok(0) => {
                    return Err(io::Error::new(
                        io::ErrorKind::UnexpectedEof,
                        "response ended",
                    ));
                }
                Ok(count) => bytes = &mut bytes[count..],
                Err(error) if error.kind() == io::ErrorKind::Interrupted => continue,
                Err(error) => return Err(error),
            }
        }
        Ok(())
    }
}

fn control_request(command: u8) -> [u8; 4] {
    [4, command, 0, 4u8.wrapping_add(command)]
}

fn invalid(message: impl Into<String>) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message.into())
}

fn at(stage: &str, error: io::Error) -> io::Error {
    io::Error::new(error.kind(), format!("{stage}: {error}"))
}

fn block_at(stage: &str, offset: usize, error: io::Error) -> io::Error {
    at(&format!("{stage} at {offset:#06x}"), error)
}

#[cfg(test)]
mod tests {
    use std::cell::RefCell;
    use std::collections::VecDeque;
    use std::rc::Rc;

    use super::*;

    #[derive(Default)]
    struct Trace {
        sent: Vec<u8>,
        input: VecDeque<Option<u8>>,
    }

    struct TestLink<'a>(&'a mut Trace);

    impl Link for TestLink<'_> {
        fn set_read_timeout(&mut self, _: Duration) -> io::Result<()> {
            Ok(())
        }
    }

    impl Read for TestLink<'_> {
        fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
            let byte = self
                .0
                .input
                .pop_front()
                .flatten()
                .ok_or_else(|| io::Error::new(io::ErrorKind::TimedOut, "no test response"))?;
            buf[0] = byte;
            Ok(1)
        }
    }

    impl Write for TestLink<'_> {
        fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
            self.0.sent.extend_from_slice(buf);
            Ok(buf.len())
        }

        fn flush(&mut self) -> io::Result<()> {
            Ok(())
        }
    }

    #[test]
    fn retries_only_until_the_initial_bootstrap_acknowledgement() {
        let mut trace = Trace {
            input: [Some(0), None, Some(0), Some(0x55), None].into(),
            ..Trace::default()
        };
        let boot = [0; BOOT_FILE_SIZE];
        let error = Session::start_waiting(
            TestLink(&mut trace),
            &Bootstrap(&boot),
            Duration::from_secs(2),
            |_| {},
        )
        .err()
        .unwrap();
        assert_eq!(trace.sent, [0, 0, 0]);
        assert_eq!(error.kind(), io::ErrorKind::TimedOut);
        assert!(error.to_string().starts_with("bootstrap frame 1"));
    }

    #[test]
    fn reports_erase_complete_only_after_the_erase_acknowledgement() {
        for confirmed in [false, true] {
            let mut trace = Trace::default();
            let mut reply = vec![4, 0x11, 0, 0x15, 5, 0x91, 0, 0x80, 0x16, 4, 0x12, 0, 0x16];
            if confirmed {
                reply.extend_from_slice(&[4, 0x92, 0x55, 0xeb]);
            }
            trace.input.extend(reply.into_iter().map(Some));
            let progress = Rc::new(RefCell::new(Vec::new()));
            let captured = Rc::clone(&progress);
            let mut session = Session {
                link: TestLink(&mut trace),
                flash_probe: None,
                progress: Some(Box::new(move |event| captured.borrow_mut().push(event))),
            };
            assert_eq!(session.clear_flash().is_ok(), confirmed);
            let progress = progress.borrow();
            assert_eq!(progress.len(), if confirmed { 2 } else { 1 });
            assert_eq!(progress.last().unwrap().stage, Stage::EraseFlash);
            assert_eq!(progress.last().unwrap().completed, usize::from(confirmed));
        }
    }

    #[test]
    fn reuses_the_verified_flag_zero_probe_before_erasing_flash() {
        let mut trace = Trace::default();
        trace.input.extend(
            [
                4, 0x13, 0, 0x17, 5, 0x93, 0x55, 0, 0xed, 4, 0x11, 0, 0x15, 5, 0x91, 0, 0x02, 0x98,
                4, 0x12, 0, 0x16, 4, 0x92, 0x55, 0xeb,
            ]
            .into_iter()
            .map(Some),
        );
        let mut session = Session {
            link: TestLink(&mut trace),
            flash_probe: None,
            progress: None,
        };
        session.identify_flash().unwrap();
        assert_eq!(session.flash_probe(), Some([0, 2]));
        session.clear_flash().unwrap();
        assert_eq!(
            trace.sent,
            [4, 0x13, 0, 0x17, 4, 0x11, 0, 0x15, 4, 0x12, 0, 0x16]
        );
    }
}
