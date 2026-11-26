//! The C509 bootstrap and the CombiLoader programming exchange.

use std::io::{self, Read, Write};
use std::thread;
use std::time::{Duration, Instant};

use janvar_editor::firmware::IMAGE_SIZE;
use sha2::{Digest, Sha256};

use crate::{BLOCK_SIZE, EEPROM_SIZE, WriteImage, read, write};

mod protocol;
use protocol::{Command, Echo, Request};

const BLOCK_TIMEOUT: Duration = Duration::from_secs(2);
const PROBE_TIMEOUT: Duration = Duration::from_millis(500);
const PROBE_INTERVAL: Duration = Duration::from_millis(250);
const BOOT_HEADER_END: usize = 9;
const BOOT_FOOTER_SIZE: usize = 8;
const BOOT_FRAME_SIZE: usize = 18;
const BOOT_FILE_SIZE: usize = BOOT_HEADER_END + 177 * BOOT_FRAME_SIZE + BOOT_FOOTER_SIZE;
const EMBEDDED_BOOTSTRAP: &[u8; BOOT_FILE_SIZE] = include_bytes!(env!("JANVAR_BOOTSTRAP_FRAMES"));
const BOOT_SHA256: [u8; 32] = [
    0xa0, 0x43, 0xab, 0x3c, 0x22, 0x27, 0xe8, 0x4c, 0xa6, 0xc3, 0xf8, 0x75, 0x09, 0x1a, 0xb4, 0x74,
    0x7b, 0xe8, 0x15, 0x34, 0x53, 0x3b, 0xf8, 0xd8, 0x1d, 0xc4, 0x08, 0xf4, 0xb2, 0x5a, 0xd7, 0x89,
];

/// The verified embedded C509-L bootstrap byte stream.
pub struct Bootstrap(&'static [u8; BOOT_FILE_SIZE]);

impl Bootstrap {
    pub fn embedded() -> io::Result<Self> {
        if Sha256::digest(EMBEDDED_BOOTSTRAP)[..] != BOOT_SHA256 {
            return Err(invalid("unexpected embedded bootstrap SHA-256"));
        }
        Ok(Self(EMBEDDED_BOOTSTRAP))
    }

    fn frames(&self) -> impl Iterator<Item = &[u8]> {
        let data_end = self.0.len() - BOOT_FOOTER_SIZE;
        let (header, data) = self.0[..data_end].split_at(BOOT_HEADER_END);
        let footer = &self.0[data_end..];
        std::iter::once(&header[1..])
            .chain(
                data.as_chunks::<BOOT_FRAME_SIZE>()
                    .0
                    .iter()
                    .map(<[_; BOOT_FRAME_SIZE]>::as_slice),
            )
            .chain(std::iter::once(footer))
    }
}

pub trait Link: Read + Write {
    fn set_read_timeout(&mut self, timeout: Duration) -> io::Result<()>;

    fn discard_input(&mut self) -> io::Result<()> {
        Ok(())
    }
}

#[cfg(windows)]
pub use serialport::COMPort as NativePort;
#[cfg(any(target_os = "macos", target_os = "linux"))]
pub use serialport::TTYPort as NativePort;

#[cfg(any(windows, target_os = "macos", target_os = "linux"))]
impl Link for NativePort {
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
    controller_flags: u8,
    flash_probe: Option<[u8; 2]>,
    progress: Option<Box<dyn FnMut(Progress)>>,
}

impl<T: Link> Session<T> {
    pub fn start(link: T, bootstrap: &Bootstrap) -> io::Result<Self> {
        let mut session = Self {
            link,
            controller_flags: 0,
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
        bootstrap: &Bootstrap,
        timeout: Duration,
        progress: impl FnMut(Progress) + 'static,
    ) -> io::Result<Self> {
        let mut session = Self {
            link,
            controller_flags: 0,
            flash_probe: None,
            progress: Some(Box::new(progress)),
        };
        session.wait_for_connection(timeout)?;
        session.finish_start(bootstrap)
    }

    fn finish_start(mut self, bootstrap: &Bootstrap) -> io::Result<Self> {
        let total = bootstrap.frames().count();
        self.report(Stage::Bootstrap, 0, total);
        for (index, frame) in bootstrap.frames().enumerate() {
            let completed = index + 1;
            self.send_bootstrap_frame(frame)
                .map_err(|error| at(&format!("bootstrap frame {completed}"), error))?;
            self.report(Stage::Bootstrap, completed, total);
        }
        // Let the uploaded code complete its board and Flash probes.
        thread::sleep(Duration::from_millis(100));
        self.identify_controller()
            .map_err(|error| at("identify controller", error))?;
        Ok(self)
    }

    fn identify_controller(&mut self) -> io::Result<()> {
        let [status, flags] = self.control(Command::ControllerFlags)?;
        require_ack(Command::ControllerFlags, status)?;
        match flags {
            0x00 | 0x04 => {
                self.controller_flags = flags;
                Ok(())
            }
            _ => Err(invalid(format!(
                "unsupported controller flags {flags:#04x}"
            ))),
        }
    }

    pub fn controller_flags(&self) -> u8 {
        self.controller_flags
    }

    /// Returns the two bytes reported by the last probe before a Flash erase.
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
        let [layout] = self
            .control(Command::FlashLayout)
            .map_err(|error| at("identify Flash layout", error))?;
        if layout != 0x55 {
            return Err(invalid(format!(
                "Flash layout {layout:#04x} requires a 128 KiB read; only the captured 64 KiB mode is supported"
            )));
        }

        let mut image = Vec::with_capacity(IMAGE_SIZE);
        let total = IMAGE_SIZE / BLOCK_SIZE;
        self.report(Stage::ReadFlash, 0, total);
        for offset in (0..IMAGE_SIZE).step_by(BLOCK_SIZE) {
            let request = read::request(offset).map_err(io::Error::other)?;
            let data: [u8; BLOCK_SIZE] = self
                .exchange(Command::ReadFlash, &request, Echo::Optional)
                .map_err(|error| block_at("read Flash", offset, error))?;
            image.extend_from_slice(&data);
            self.report(Stage::ReadFlash, image.len() / BLOCK_SIZE, total);
        }
        Ok(image)
    }

    pub fn clear_flash(&mut self) -> io::Result<()> {
        // CombiLoader probes immediately before erase, regardless of controller flags.
        self.flash_probe = None;
        let probe = self
            .control(Command::FlashProbe)
            .map_err(|error| at("probe Flash before erase", error))?;
        self.flash_probe = Some(probe);
        self.report(Stage::EraseFlash, 0, 1);
        self.control_ack(Command::EraseFlash).map_err(|error| {
            at(
                &format!("erase Flash (probe {:02X} {:02X})", probe[0], probe[1]),
                error,
            )
        })?;
        self.report(Stage::EraseFlash, 1, 1);
        Ok(())
    }

    pub fn write_flash(&mut self, image: &WriteImage) -> io::Result<()> {
        // This check precedes any erase in the CombiLoader flag-04 write branch.
        const PROTECTION_MARKER: [u8; 4] = [0x23, 0x03, 0x33, 0x13];
        if self.controller_flags == 0x04
            && [0x6000, 0xc000]
                .into_iter()
                .any(|offset| image.bytes()[offset..].starts_with(&PROTECTION_MARKER))
        {
            return Err(invalid(
                "controller flags 0x04 do not support this protected Flash image",
            ));
        }

        self.clear_flash()?;
        let (blocks, _) = image.bytes().as_chunks::<BLOCK_SIZE>();
        self.report(Stage::WriteFlash, 0, blocks.len());
        for (block, data) in blocks.iter().enumerate() {
            let offset = block * BLOCK_SIZE;
            let request = write::request(offset, data).map_err(io::Error::other)?;
            self.exchange_ack(Command::WriteFlash, &request)
                .map_err(|error| block_at("write Flash", offset, error))?;
            self.report(Stage::WriteFlash, block + 1, blocks.len());
        }
        self.report(Stage::FinishFlash, 0, 1);
        self.control_ack(Command::FinalizeFlash)
            .map_err(|error| at("copy Flash banks", error))?;
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
            let request = Request::new(Command::ReadEeprom, &[high, low, BLOCK_SIZE as u8], &[]);
            let response: [u8; BLOCK_SIZE + 1] = self
                .exchange(Command::ReadEeprom, request.bytes(), Echo::Required)
                .map_err(|error| block_at("read EEPROM", offset, error))?;
            let [status, payload @ ..] = response;
            require_ack(Command::ReadEeprom, status)
                .map_err(|error| block_at("read EEPROM", offset, error))?;
            *data = payload;
            self.report(Stage::ReadEeprom, block + 1, total);
        }
        Ok(image)
    }

    pub fn write_eeprom(&mut self, image: &[u8; EEPROM_SIZE]) -> io::Result<()> {
        self.transfer_eeprom(image, Stage::WriteEeprom, "write EEPROM")
    }

    pub fn clear_eeprom(&mut self) -> io::Result<()> {
        self.transfer_eeprom(&[0xff; EEPROM_SIZE], Stage::ClearEeprom, "clear EEPROM")
    }

    fn transfer_eeprom(
        &mut self,
        image: &[u8; EEPROM_SIZE],
        stage: Stage,
        label: &str,
    ) -> io::Result<()> {
        let (blocks, _) = image.as_chunks::<BLOCK_SIZE>();
        self.report(stage, 0, blocks.len());
        for (block, data) in blocks.iter().enumerate() {
            let offset = block * BLOCK_SIZE;
            let [high, low] = (offset as u16).to_be_bytes();
            let request = Request::new(Command::WriteEeprom, &[high, low, BLOCK_SIZE as u8], data);
            self.exchange_ack(Command::WriteEeprom, request.bytes())
                .map_err(|error| block_at(label, offset, error))?;
            self.report(stage, block + 1, blocks.len());
        }
        Ok(())
    }

    fn send_bootstrap_frame(&mut self, frame: &[u8]) -> io::Result<()> {
        for &byte in frame {
            self.send(&[byte])?;
            self.expect(&[byte], Instant::now() + BLOCK_TIMEOUT)?;
        }
        self.expect(&[0x55], Instant::now() + BLOCK_TIMEOUT)
    }

    fn control<const N: usize>(&mut self, command: Command) -> io::Result<[u8; N]> {
        let request = Request::new(command, &[0], &[]);
        self.exchange(command, request.bytes(), Echo::Required)
    }

    fn control_ack(&mut self, command: Command) -> io::Result<()> {
        let [status] = self.control(command)?;
        require_ack(command, status)
    }

    fn exchange_ack(&mut self, command: Command, request: &[u8]) -> io::Result<()> {
        let [status] = self.exchange(command, request, Echo::Required)?;
        require_ack(command, status)
    }

    fn exchange<const N: usize>(
        &mut self,
        command: Command,
        request: &[u8],
        echo: Echo,
    ) -> io::Result<[u8; N]> {
        for attempt in 1..=command.attempts() {
            self.send(request)?;
            let deadline = Instant::now() + command.timeout();
            let reply = self.receive_reply(request, echo, deadline)?;
            match protocol::payload(command, reply.bytes()) {
                Ok(data) => return Ok(data),
                // A complete, bounded frame has been consumed. It is safe to
                // retry a read without flushing a partial response.
                Err(_) if attempt < command.attempts() => {}
                Err(error) => return Err(error),
            }
        }
        unreachable!("each command has at least one attempt")
    }

    fn receive_reply(
        &mut self,
        request: &[u8],
        echo: Echo,
        deadline: Instant,
    ) -> io::Result<protocol::Reply> {
        let mut first = [0];
        if echo == Echo::Required {
            self.expect(request, deadline)
                .map_err(|error| at("command echo", error))?;
            self.receive(&mut first, deadline)
                .map_err(|error| at("reply header", error))?;
        } else {
            self.receive(&mut first, deadline)?;
            if first[0] == request[0] {
                self.expect(&request[1..], deadline)?;
                self.receive(&mut first, deadline)?;
            }
        }
        let mut reply = protocol::Reply::new(first[0])?;
        self.receive(reply.remaining(), deadline)?;
        Ok(reply)
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

fn require_ack(command: Command, status: u8) -> io::Result<()> {
    if status != 0x55 {
        return Err(invalid(format!(
            "ECU rejected command {:02X} with status {status:02X}",
            command as u8
        )));
    }
    Ok(())
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
    fn embedded_bootstrap_matches_the_verified_stream() {
        let bootstrap = Bootstrap::embedded().unwrap();
        assert_eq!(bootstrap.frames().count(), 179);
    }

    #[test]
    fn retries_only_until_the_initial_bootstrap_acknowledgement() {
        let mut trace = Trace {
            input: [Some(0), None, Some(0), Some(0x55), None].into(),
            ..Trace::default()
        };
        let bootstrap = Bootstrap::embedded().unwrap();
        let error = Session::start_waiting(
            TestLink(&mut trace),
            &bootstrap,
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
                controller_flags: 0,
                flash_probe: None,
                progress: Some(Box::new(move |event| captured.borrow_mut().push(event))),
            };
            assert_eq!(session.clear_flash().is_ok(), confirmed);
            let progress = progress.borrow();
            assert_eq!(progress.len(), if confirmed { 2 } else { 1 });
            assert_eq!(progress.last().unwrap().stage, Stage::EraseFlash);
            assert_eq!(progress.last().unwrap().completed, usize::from(confirmed));
            assert_eq!(trace.sent, [4, 0x11, 0, 0x15, 4, 0x12, 0, 0x16]);
        }
    }

    #[test]
    fn probes_flag_zero_only_when_erasing_flash() {
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
            controller_flags: 0,
            flash_probe: None,
            progress: None,
        };
        session.identify_controller().unwrap();
        assert_eq!(session.controller_flags(), 0);
        assert_eq!(session.flash_probe(), None);
        session.clear_flash().unwrap();
        assert_eq!(session.flash_probe(), Some([0, 2]));
        assert_eq!(
            trace.sent,
            [4, 0x13, 0, 0x17, 4, 0x11, 0, 0x15, 4, 0x12, 0, 0x16]
        );
    }

    #[test]
    fn retries_a_complete_corrupt_read_reply() {
        let mut trace = Trace::default();
        let request = [4, 0x13, 0, 0x17];
        trace.input.extend(
            [
                request.as_slice(),
                &[5, 0x93, 0x55, 4, 0],
                &request,
                &[5, 0x93, 0x55, 4, 0xf1],
            ]
            .into_iter()
            .flat_map(|bytes| bytes.iter().copied().map(Some)),
        );
        let mut session = Session {
            link: TestLink(&mut trace),
            controller_flags: 0,
            flash_probe: None,
            progress: None,
        };
        session.identify_controller().unwrap();
        assert_eq!(session.controller_flags(), 4);
        assert_eq!(trace.sent, request.repeat(2));
    }

    #[cfg(any(target_os = "macos", target_os = "linux"))]
    #[test]
    fn native_tty_transfers_raw_bytes_and_times_out_when_idle() {
        use serialport::SerialPort;

        let (mut peer, mut port) = NativePort::pair().unwrap();
        peer.set_timeout(Duration::from_secs(1)).unwrap();
        port.set_read_timeout(Duration::from_secs(1)).unwrap();
        port.discard_input().unwrap();

        let frame = [4, 0x13, 0, 0x17];
        port.write_all(&frame).unwrap();
        let mut received = [0; 4];
        peer.read_exact(&mut received).unwrap();
        assert_eq!(received, frame);

        let response = [5, 0x93, 0x55, 4, 0xf1];
        peer.write_all(&response).unwrap();
        let mut received = [0; 5];
        port.read_exact(&mut received).unwrap();
        assert_eq!(received, response);

        port.set_read_timeout(Duration::from_millis(10)).unwrap();
        assert_eq!(
            port.read(&mut received).unwrap_err().kind(),
            io::ErrorKind::TimedOut
        );
    }

    #[test]
    fn rejects_a_protected_image_before_sending_an_erase() {
        use janvar_editor::firmware::Image;

        let mut bytes = include_bytes!("../../../bins/J5v05l19.bin").to_vec();
        bytes[0xc000..0xc004].copy_from_slice(&[0x23, 0x03, 0x33, 0x13]);
        let image = WriteImage::new(Image::new(bytes).unwrap().to_bin()).unwrap();
        let mut trace = Trace::default();
        let mut session = Session {
            link: TestLink(&mut trace),
            controller_flags: 4,
            flash_probe: None,
            progress: None,
        };
        let error = session.write_flash(&image).unwrap_err();
        assert!(error.to_string().contains("protected Flash image"));
        assert!(trace.sent.is_empty());
    }
}
