//! Read and edit Janvar 5.1 calibration data in supported BIN images.
//!
//! The library provides firmware profiles, calibration encoding, checksum
//! calculation, and CTM parsers. It can be used without a desktop environment.
//!
//! ```no_run
//! use janvar_editor::firmware::{Image, Parameter, SaveMode};
//!
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! let bytes = std::fs::read("original.bin")?;
//! let mut image = Image::new(bytes)?;
//! image.set(Parameter::FanOn, 0, 102.0)?;
//! image.save("edited.bin".as_ref(), SaveMode::Create)?;
//! # Ok(())
//! # }
//! ```

pub mod firmware;
pub mod j5;
