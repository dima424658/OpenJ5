use std::env;
use std::ffi::OsString;
use std::fs;
use std::io::{self, IsTerminal, Write};
use std::path::PathBuf;
use std::time::Duration;

use crate::prepare::parse_flag;
use janvar_flash::advanced::FlashOptions;
use janvar_flash::programmer::{
    self, DEFAULT_CONNECTION_TIMEOUT, Event, Operation, OperationError,
};
use janvar_flash::session::{Progress, Stage};

type Result<T> = std::result::Result<T, OperationError>;

const USAGE: &str = "Usage:
  janvar-flash read         --port PORT --out firmware.bin
  janvar-flash write        --port PORT --input firmware.bin
  janvar-flash clear-flash  --port PORT
  janvar-flash read-eeprom  --port PORT --out backup.eep
  janvar-flash write-eeprom --port PORT --input backup.eep
  janvar-flash clear-eeprom --port PORT
  janvar-flash prepare-flash --help

Flash write accepts --program-original, --remove-calibration-checks,
--remove-error-mask, --fix-checksum HEX and --starter-relay TEMPLATE.

PORT is a serial device, for example COM1, /dev/cu.usbserial-A10, or /dev/ttyUSB0.
Serial operations use the embedded C509-L bootstrap.
--connect-timeout SECONDS sets the ECU wait for each connection (default 60).
Writes and clears wait for a manual ECU power cycle, then verify by reading back.";

enum Request {
    ReadFlash(PathBuf),
    WriteFlash(PathBuf),
    ClearFlash,
    ReadEeprom(PathBuf),
    WriteEeprom(PathBuf),
    ClearEeprom,
}

struct Options {
    port: String,
    connection_timeout: Duration,
    request: Request,
    flash_options: FlashOptions,
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
    let operation = match options.request {
        Request::ReadFlash(path) => Operation::ReadFlash(path),
        Request::WriteFlash(path) => {
            Operation::WriteFlash(options.flash_options.prepare(fs::read(path)?)?)
        }
        Request::ClearFlash => Operation::ClearFlash,
        Request::ReadEeprom(path) => Operation::ReadEeprom(path),
        Request::WriteEeprom(path) => {
            Operation::WriteEeprom(programmer::eeprom_image(fs::read(path)?)?)
        }
        Request::ClearEeprom => Operation::ClearEeprom,
    };
    let port = options.port.clone();
    let timeout = options.connection_timeout;
    let mut progress = ProgressPrinter {
        interactive: io::stderr().is_terminal(),
        in_line: false,
    };
    let bytes = programmer::run(
        &operation,
        &options.port,
        timeout,
        move |event| match event {
            Event::Waiting => eprintln!(
                "Waiting for ECU on {port} (up to {} s)...",
                timeout.as_secs()
            ),
            Event::Connected(flags) => eprintln!("Controller flags {flags:02X}"),
            Event::Progress(update) => progress.update(update),
            Event::AwaitingPowerCycle(status) => {
                eprintln!("{status}. Power-cycle the ECU into programming mode for readback.");
            }
        },
    )?;
    match operation {
        Operation::ReadFlash(path) => println!("Saved {bytes} Flash bytes to {}", path.display()),
        Operation::ReadEeprom(path) => println!("Saved {bytes} EEPROM bytes to {}", path.display()),
        Operation::WriteFlash(_) => println!("Flash write verified by readback ({bytes} bytes)"),
        Operation::WriteEeprom(_) => println!("EEPROM write verified by readback ({bytes} bytes)"),
        Operation::ClearFlash => println!("Flash erase verified over the {bytes}-byte image"),
        Operation::ClearEeprom => println!("EEPROM clear verified by readback ({bytes} bytes)"),
    }
    Ok(())
}

impl Options {
    fn parse(command: OsString, mut args: impl Iterator<Item = OsString>) -> Result<Self> {
        let mut port = None;
        let mut connection_timeout = None;
        let mut input = None;
        let mut output = None;
        let mut flash_options = FlashOptions::default();

        while let Some(flag) = args.next() {
            let target = match flag.to_str() {
                Some("--port") => &mut port,
                Some("--connect-timeout") => &mut connection_timeout,
                Some("--input") => &mut input,
                Some("--out") => &mut output,
                _ if parse_flag(&mut flash_options, &flag, "--program-original", &mut args)? => {
                    continue;
                }
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

        let request = match (command.to_str(), input, output) {
            (Some("read"), None, Some(path)) => Request::ReadFlash(path.into()),
            (Some("write"), Some(path), None) => Request::WriteFlash(path.into()),
            (Some("clear-flash"), None, None) => Request::ClearFlash,
            (Some("read-eeprom"), None, Some(path)) => Request::ReadEeprom(path.into()),
            (Some("write-eeprom"), Some(path), None) => Request::WriteEeprom(path.into()),
            (Some("clear-eeprom"), None, None) => Request::ClearEeprom,
            _ => return Err(format!("Invalid command or file flags\n\n{USAGE}").into()),
        };

        if flash_options.requested() && !matches!(request, Request::WriteFlash(_)) {
            return Err("Flash preparation flags require the write command".into());
        }
        let timeout = match connection_timeout {
            Some(value) => value
                .to_str()
                .and_then(|value| value.parse::<u32>().ok())
                .filter(|seconds| *seconds > 0)
                .ok_or("--connect-timeout must be a positive number of seconds")?,
            None => DEFAULT_CONNECTION_TIMEOUT.as_secs() as u32,
        };
        Ok(Self {
            port: port
                .and_then(|port| port.into_string().ok())
                .ok_or("A serial port is required (--port PORT)")?,
            connection_timeout: Duration::from_secs(u64::from(timeout)),
            request,
            flash_options,
        })
    }
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
            Stage::FinishFlash => "Copying Flash banks",
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
