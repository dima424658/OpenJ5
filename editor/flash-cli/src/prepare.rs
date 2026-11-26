use std::ffi::{OsStr, OsString};
use std::fs;
use std::path::PathBuf;

use janvar_flash::advanced::{FlashOptions, parse_checksum};

pub const PREPARE_USAGE: &str = "Usage:
  janvar-flash prepare-flash --input firmware.bin --out prepared.bin
      [--save-original] [--remove-calibration-checks] [--remove-error-mask]
      [--fix-checksum HEX] [--starter-relay TEMPLATE]";

type CliResult<T> = Result<T, janvar_flash::programmer::OperationError>;

pub fn parse_flag(
    options: &mut FlashOptions,
    flag: &OsStr,
    original_flag: &str,
    args: &mut impl Iterator<Item = OsString>,
) -> CliResult<bool> {
    if flag == original_flag {
        mark(&mut options.original, original_flag)?;
    } else {
        match flag.to_str() {
            Some("--remove-calibration-checks") => {
                mark(
                    &mut options.remove_calibration_checks,
                    "--remove-calibration-checks",
                )?;
            }
            Some("--remove-error-mask") => {
                mark(&mut options.remove_error_mask, "--remove-error-mask")?
            }
            Some("--starter-relay") => {
                if options.starter_relay.is_some() {
                    return Err("Duplicate --starter-relay".into());
                }
                let path = args.next().ok_or("Missing template for --starter-relay")?;
                options.starter_relay = Some(path.into());
            }
            Some("--fix-checksum") => {
                if options.checksum.is_some() {
                    return Err("Duplicate --fix-checksum".into());
                }
                let raw = args.next().ok_or("Missing HEX value for --fix-checksum")?;
                let raw = raw.to_str().ok_or("Expected a hexadecimal checksum")?;
                options.checksum = Some(parse_checksum(raw)?);
            }
            _ => return Ok(false),
        }
    }
    Ok(true)
}

fn mark(value: &mut bool, flag: &str) -> CliResult<()> {
    if *value {
        return Err(format!("Duplicate {flag}").into());
    }
    *value = true;
    Ok(())
}

pub fn run_prepare(mut args: impl Iterator<Item = OsString>) -> CliResult<()> {
    let mut input = None;
    let mut output = None;
    let mut options = FlashOptions::default();
    while let Some(flag) = args.next() {
        let path = match flag.to_str() {
            Some("--help" | "-h") => {
                println!("{PREPARE_USAGE}");
                return Ok(());
            }
            Some("--input") => &mut input,
            Some("--out") => &mut output,
            _ if parse_flag(&mut options, &flag, "--save-original", &mut args)? => continue,
            _ => {
                return Err(
                    format!("Unknown option {}\n{PREPARE_USAGE}", flag.to_string_lossy()).into(),
                );
            }
        };
        if path.is_some() {
            return Err(format!("Duplicate {}", flag.to_string_lossy()).into());
        }
        *path = Some(PathBuf::from(args.next().ok_or_else(|| {
            format!("Missing path for {}", flag.to_string_lossy())
        })?));
    }
    let input = input.ok_or(PREPARE_USAGE)?;
    let output = output.ok_or(PREPARE_USAGE)?;
    match options.save_prepared(fs::read(input)?, &output)? {
        None => println!("Saved original Flash to {}", output.display()),
        Some(checksum) => println!(
            "Saved Flash to {} (checksum {checksum:04X})",
            output.display()
        ),
    }
    Ok(())
}
