use std::env;
use std::error::Error;
use std::ffi::OsString;
use std::fs;
use std::io::{self, IsTerminal, Write};
use std::path::PathBuf;
use std::thread;
use std::time::Duration;

use janvar_editor::firmware::IMAGE_SIZE;
use janvar_flash::session::{Bootstrap, Progress, Session, Stage};
use janvar_flash::{EEPROM_SIZE, WriteImage, save_dump, save_eeprom};
use serialport::{COMPort, ClearBuffer, DataBits, FlowControl, Parity, SerialPort, StopBits};

type Result<T> = std::result::Result<T, Box<dyn Error>>;

const DEFAULT_CONNECTION_TIMEOUT: u32 = 60;
const USAGE: &str = "Usage:
  janvar-flash read         --port COMx --out firmware.bin
  janvar-flash write        --port COMx --input firmware.bin
  janvar-flash clear-flash  --port COMx
  janvar-flash read-eeprom  --port COMx --out backup.eep
  janvar-flash write-eeprom --port COMx --input backup.eep
  janvar-flash clear-eeprom --port COMx

All operations accept --bootstrap FILE. By default the program uses
combi-bootstrap-frames.bin next to janvar-flash.exe.
--connect-timeout SECONDS sets the ECU wait for each connection (default 60).
Writes and clears wait for a manual ECU power cycle, then verify by reading back.";

enum Operation {
    ReadFlash(PathBuf),
    WriteFlash(PathBuf),
    ClearFlash,
    ReadEeprom(PathBuf),
    WriteEeprom(PathBuf),
    ClearEeprom,
}

struct Options {
    port: String,
    bootstrap: PathBuf,
    connection_timeout: Duration,
    operation: Operation,
}

pub fn run() -> Result<()> {
    let mut args = env::args_os().skip(1);
    let Some(command) = args.next() else {
        println!("{USAGE}");
        return Ok(());
    };
    if command == "--help" || command == "-h" {
        println!("{USAGE}");
        return Ok(());
    }
    let options = Options::parse(command, args)?;
    let boot_bytes = fs::read(&options.bootstrap)
        .map_err(|error| format!("Read bootstrap {}: {error}", options.bootstrap.display()))?;
    let bootstrap = Bootstrap::new(&boot_bytes)?;
    let port = &options.port;
    let connect = || connect(port, &bootstrap, options.connection_timeout);
    let reconnect = |status| {
        eprintln!("{status}. Power-cycle the ECU into programming mode for readback.");
        connect().map_err(|error| format!("{status}; readback connection failed: {error}"))
    };

    match options.operation {
        Operation::ReadFlash(path) => {
            let image = connect()?.read_flash()?;
            save_dump(&path, &image)?;
            println!("Saved {} Flash bytes to {}", image.len(), path.display());
        }
        Operation::WriteFlash(path) => {
            let image = WriteImage::new(fs::read(&path)?)?;
            connect()?.write_flash(&image)?;
            let readback = reconnect("Flash write acknowledged")?.read_flash()?;
            image.verify_readback(&readback)?;
            println!(
                "Flash write verified by readback ({} bytes)",
                readback.len()
            );
        }
        Operation::ClearFlash => {
            connect()?.clear_flash()?;
            let readback = reconnect("Flash erase acknowledged")?.read_flash()?;
            verify(&[0xff; IMAGE_SIZE], &readback)?;
            println!(
                "Flash erase verified over the {}-byte image",
                readback.len()
            );
        }
        Operation::ReadEeprom(path) => {
            let image = connect()?.read_eeprom()?;
            save_eeprom(&path, &image)?;
            println!("Saved {} EEPROM bytes to {}", image.len(), path.display());
        }
        Operation::WriteEeprom(path) => {
            let image: [u8; EEPROM_SIZE] =
                fs::read(&path)?.try_into().map_err(|bytes: Vec<u8>| {
                    format!("Expected {EEPROM_SIZE} EEPROM bytes, got {}", bytes.len())
                })?;
            connect()?.write_eeprom(&image)?;
            let readback = reconnect("EEPROM write acknowledged")?.read_eeprom()?;
            verify(&image, &readback)?;
            println!("EEPROM write verified by readback ({EEPROM_SIZE} bytes)");
        }
        Operation::ClearEeprom => {
            connect()?.clear_eeprom()?;
            let readback = reconnect("EEPROM clear acknowledged")?.read_eeprom()?;
            verify(&[0xff; EEPROM_SIZE], &readback)?;
            println!("EEPROM clear verified by readback ({EEPROM_SIZE} bytes)");
        }
    }
    Ok(())
}

impl Options {
    fn parse(command: OsString, mut args: impl Iterator<Item = OsString>) -> Result<Self> {
        let mut port = None;
        let mut bootstrap = None;
        let mut connection_timeout = None;
        let mut input = None;
        let mut output = None;

        while let Some(flag) = args.next() {
            let target = match flag.to_str() {
                Some("--port") => &mut port,
                Some("--bootstrap") => &mut bootstrap,
                Some("--connect-timeout") => &mut connection_timeout,
                Some("--input") => &mut input,
                Some("--out") => &mut output,
                _ => {
                    return Err(
                        format!("Unknown flag {}\n\n{USAGE}", flag.to_string_lossy()).into(),
                    );
                }
            };
            let value = args
                .next()
                .filter(|value| !value.is_empty() && !value.to_string_lossy().starts_with("--"))
                .ok_or_else(|| format!("Missing value for {}", flag.to_string_lossy()))?;
            if target.replace(value).is_some() {
                return Err(format!("Duplicate flag {}", flag.to_string_lossy()).into());
            }
        }

        let operation = match (command.to_str(), input, output) {
            (Some("read"), None, Some(path)) => Operation::ReadFlash(path.into()),
            (Some("write"), Some(path), None) => Operation::WriteFlash(path.into()),
            (Some("clear-flash"), None, None) => Operation::ClearFlash,
            (Some("read-eeprom"), None, Some(path)) => Operation::ReadEeprom(path.into()),
            (Some("write-eeprom"), Some(path), None) => Operation::WriteEeprom(path.into()),
            (Some("clear-eeprom"), None, None) => Operation::ClearEeprom,
            _ => return Err(format!("Invalid command or file flags\n\n{USAGE}").into()),
        };

        let timeout = match connection_timeout {
            Some(value) => value
                .to_str()
                .and_then(|value| value.parse::<u32>().ok())
                .filter(|seconds| *seconds > 0)
                .ok_or("--connect-timeout must be a positive number of seconds")?,
            None => DEFAULT_CONNECTION_TIMEOUT,
        };
        Ok(Self {
            port: port
                .and_then(|port| port.into_string().ok())
                .ok_or("A COM port is required (--port COMx)")?,
            bootstrap: match bootstrap {
                Some(path) => path.into(),
                None => env::current_exe()?.with_file_name("combi-bootstrap-frames.bin"),
            },
            connection_timeout: Duration::from_secs(u64::from(timeout)),
            operation,
        })
    }
}

fn connect(port: &str, bootstrap: &Bootstrap<'_>, timeout: Duration) -> Result<Session<COMPort>> {
    let mut link = serialport::new(port, 38_400)
        .data_bits(DataBits::Eight)
        .parity(Parity::None)
        .stop_bits(StopBits::Two)
        .flow_control(FlowControl::None)
        .timeout(Duration::from_secs(2))
        .open_native()
        .map_err(|error| format!("Open {port}: {error}"))?;
    link.write_request_to_send(true)?;
    link.write_data_terminal_ready(false)?;
    thread::sleep(Duration::from_millis(500));
    link.clear(ClearBuffer::All)?;
    eprintln!(
        "Waiting for ECU on {port} (up to {} s)...",
        timeout.as_secs()
    );
    let mut progress = ProgressPrinter {
        interactive: io::stderr().is_terminal(),
        in_line: false,
    };
    let session = Session::start_waiting(link, bootstrap, timeout, move |event| {
        progress.update(event)
    })?;
    if let Some([first, second]) = session.flash_probe() {
        eprintln!("Flash flag 00, probe bytes {first:02X} {second:02X}");
    }
    Ok(session)
}

struct ProgressPrinter {
    interactive: bool,
    in_line: bool,
}

impl ProgressPrinter {
    fn update(&mut self, progress: Progress) {
        let Progress {
            stage,
            completed,
            total,
        } = progress;
        let label = match stage {
            Stage::Bootstrap => "Loading bootstrap",
            Stage::ReadFlash => "Reading Flash",
            Stage::EraseFlash => "Erasing Flash",
            Stage::WriteFlash => "Writing Flash",
            Stage::FinishFlash => "Finalizing Flash write",
            Stage::ReadEeprom => "Reading EEPROM",
            Stage::WriteEeprom => "Writing EEPROM",
            Stage::ClearEeprom => "Clearing EEPROM",
        };

        if total == 1 {
            let status = if completed == 0 {
                "waiting for ECU"
            } else {
                "acknowledged"
            };
            eprintln!("{label} — {status}");
            return;
        }
        if completed != total && !completed.is_multiple_of(total.div_ceil(20)) {
            return;
        }
        let percent = 100 * completed / total;
        let message = format!("{label} {percent:3}% ({completed}/{total})");
        if self.interactive {
            eprint!("\r{message:<54}");
            self.in_line = completed != total;
            if !self.in_line {
                eprintln!();
            }
            let _ = io::stderr().flush();
        } else {
            eprintln!("{message}");
        }
    }
}

impl Drop for ProgressPrinter {
    fn drop(&mut self) {
        if self.in_line {
            eprintln!();
        }
    }
}

fn verify(expected: &[u8], actual: &[u8]) -> Result<()> {
    if expected.len() != actual.len() {
        return Err(format!(
            "Readback: expected {} bytes, got {}",
            expected.len(),
            actual.len()
        )
        .into());
    }
    for (offset, (&want, &got)) in expected.iter().zip(actual).enumerate() {
        if want != got {
            return Err(format!(
                "Readback mismatch at {offset:#06x}: expected {want:02x}, got {got:02x}"
            )
            .into());
        }
    }
    Ok(())
}
