//! Serial programming operations shared by the editor and the CLI.

use std::path::PathBuf;
use std::time::Duration;

use crate::session::Progress;
use crate::{EEPROM_SIZE, WriteImage};

pub const DEFAULT_CONNECTION_TIMEOUT: Duration = Duration::from_secs(60);

pub type OperationError = Box<dyn std::error::Error + Send + Sync>;

pub enum Operation {
    ReadFlash(PathBuf),
    WriteFlash(WriteImage),
    ClearFlash,
    ReadEeprom(PathBuf),
    WriteEeprom(Box<[u8; EEPROM_SIZE]>),
    ClearEeprom,
}

#[derive(Clone, Copy, Debug)]
pub enum Event {
    Waiting,
    Connected(u8),
    Progress(Progress),
    AwaitingPowerCycle(&'static str),
}

pub fn eeprom_image(bytes: Vec<u8>) -> Result<Box<[u8; EEPROM_SIZE]>, OperationError> {
    bytes
        .into_boxed_slice()
        .try_into()
        .map_err(|bytes: Box<[u8]>| {
            format!("Expected {EEPROM_SIZE} EEPROM bytes, got {}", bytes.len()).into()
        })
}

#[cfg(any(windows, target_os = "macos", target_os = "linux"))]
mod serial {
    use super::*;
    use crate::session::{Bootstrap, NativePort, Session};
    use crate::{save_dump, save_eeprom};
    use janvar_editor::firmware::IMAGE_SIZE;
    use serialport::{ClearBuffer, DataBits, FlowControl, Parity, SerialPort, StopBits};
    use std::{cell::RefCell, rc::Rc, thread};

    pub fn ports() -> Result<Vec<String>, OperationError> {
        let mut ports: Vec<_> = serialport::available_ports()?
            .into_iter()
            .map(|port| port.port_name)
            .collect();
        ports.sort();
        ports.dedup();
        Ok(ports)
    }

    pub fn run(
        operation: &Operation,
        port: &str,
        timeout: Duration,
        report: impl FnMut(Event) + 'static,
    ) -> Result<usize, OperationError> {
        let bootstrap = Bootstrap::embedded()?;
        let report = Rc::new(RefCell::new(report));
        let open = || connect(port, &bootstrap, timeout, Rc::clone(&report));
        let reopen = |status| {
            report.borrow_mut()(Event::AwaitingPowerCycle(status));
            open().map_err(|error| -> OperationError {
                format!("{status}; readback connection failed: {error}").into()
            })
        };

        match operation {
            Operation::ReadFlash(path) => {
                let image = open()?.read_flash()?;
                save_dump(path, &image)?;
                Ok(image.len())
            }
            Operation::WriteFlash(image) => {
                open()?.write_flash(image)?;
                let actual = reopen("Flash write acknowledged")?.read_flash()?;
                image.verify_readback(&actual)?;
                Ok(actual.len())
            }
            Operation::ClearFlash => {
                open()?.clear_flash()?;
                let actual = reopen("Flash erase acknowledged")?.read_flash()?;
                verify(&[0xff; IMAGE_SIZE], &actual)?;
                Ok(actual.len())
            }
            Operation::ReadEeprom(path) => {
                let image = open()?.read_eeprom()?;
                save_eeprom(path, &image)?;
                Ok(image.len())
            }
            Operation::WriteEeprom(image) => {
                open()?.write_eeprom(image)?;
                let actual = reopen("EEPROM write acknowledged")?.read_eeprom()?;
                verify(image.as_slice(), &actual)?;
                Ok(actual.len())
            }
            Operation::ClearEeprom => {
                open()?.clear_eeprom()?;
                let actual = reopen("EEPROM clear acknowledged")?.read_eeprom()?;
                verify(&[0xff; EEPROM_SIZE], &actual)?;
                Ok(actual.len())
            }
        }
    }

    fn connect(
        port: &str,
        bootstrap: &Bootstrap,
        timeout: Duration,
        report: Rc<RefCell<impl FnMut(Event) + 'static>>,
    ) -> Result<Session<NativePort>, OperationError> {
        let mut link = serialport::new(port, 38_400)
            .data_bits(DataBits::Eight)
            .parity(Parity::None)
            .stop_bits(StopBits::Two)
            .flow_control(FlowControl::None)
            .timeout(Duration::from_secs(2))
            .open_native()
            .map_err(|error| format!("Open {port}: {error}"))?;
        link.write_request_to_send(true)
            .map_err(|error| format!("{port}: set RTS: {error}"))?;
        link.write_data_terminal_ready(false)
            .map_err(|error| format!("{port}: clear DTR: {error}"))?;
        thread::sleep(Duration::from_millis(500));
        link.clear(ClearBuffer::All)?;
        report.borrow_mut()(Event::Waiting);
        let events = Rc::clone(&report);
        let session = Session::start_waiting(link, bootstrap, timeout, move |progress| {
            events.borrow_mut()(Event::Progress(progress));
        })?;
        report.borrow_mut()(Event::Connected(session.controller_flags()));
        Ok(session)
    }

    fn verify(expected: &[u8], actual: &[u8]) -> Result<(), OperationError> {
        if expected.len() != actual.len() {
            return Err(format!(
                "Readback: expected {} bytes, got {}",
                expected.len(),
                actual.len()
            )
            .into());
        }
        if let Some((offset, (&want, &got))) = expected
            .iter()
            .zip(actual)
            .enumerate()
            .find(|(_, (want, got))| want != got)
        {
            return Err(format!(
                "Readback mismatch at {offset:#06x}: expected {want:02x}, got {got:02x}"
            )
            .into());
        }
        Ok(())
    }
}

#[cfg(any(windows, target_os = "macos", target_os = "linux"))]
pub use serial::{ports, run};

#[cfg(not(any(windows, target_os = "macos", target_os = "linux")))]
pub fn ports() -> Result<Vec<String>, OperationError> {
    Err("Serial ports require Windows, macOS, or Linux".into())
}

#[cfg(not(any(windows, target_os = "macos", target_os = "linux")))]
pub fn run(
    _operation: &Operation,
    _port: &str,
    _timeout: Duration,
    _report: impl FnMut(Event) + 'static,
) -> Result<usize, OperationError> {
    Err("Serial programming requires Windows, macOS, or Linux".into())
}
