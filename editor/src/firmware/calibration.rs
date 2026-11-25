use super::{FirmwareError, quantize_raw};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Parameter {
    Ignition,
    Idle,
    FanOn,
    FanOff,
    AirCharge,
}

#[derive(Clone, Copy)]
pub struct Spec {
    pub id: [u8; 4],

    pub address: usize,
    pub shape: &'static [usize],
    pub unit: &'static str,
    pub min: f64,
    pub max: f64,
    pub step: f64,
}

impl Parameter {
    pub const ALL: [Self; 5] = [
        Self::Ignition,
        Self::Idle,
        Self::FanOn,
        Self::FanOff,
        Self::AirCharge,
    ];

    pub fn spec(self) -> Spec {
        match self {
            Self::Ignition => Spec {
                id: [0x32, 0x28, 0x79, 0x9F],

                address: 0x6AC8,
                shape: &[16, 16],
                unit: "° п.к.в.",
                min: -64.0,
                max: 63.5,
                step: 0.5,
            },
            Self::Idle => Spec {
                id: [0xD5, 0x1C, 0xF6, 0x10],

                address: 0x6F56,
                shape: &[39],
                unit: "об/мин",
                min: 0.0,
                max: 2550.0,
                step: 10.0,
            },
            Self::FanOn => Spec {
                id: [0x03, 0xD9, 0xCA, 0x09],

                address: 0x60C8,
                shape: &[],
                unit: "°C",
                min: -40.0,
                max: 215.0,
                step: 1.0,
            },
            Self::FanOff => Spec {
                id: [0xCE, 0xF5, 0x66, 0xCE],

                address: 0x60C7,
                shape: &[],
                unit: "°C",
                min: -40.0,
                max: 215.0,
                step: 1.0,
            },
            Self::AirCharge => Spec {
                id: [0x4E, 0x7F, 0x9E, 0x27],

                address: 0x75EF,
                shape: &[16, 16],
                unit: "raw",
                min: 0.0,
                max: 255.0,
                step: 1.0,
            },
        }
    }

    pub fn count(self) -> usize {
        self.spec().shape.iter().product()
    }

    pub fn writable(self) -> bool {
        self != Self::AirCharge
    }

    pub fn decode(self, raw: u8) -> f64 {
        match self {
            Self::Ignition => f64::from(raw as i8) * 0.5,
            Self::Idle => f64::from(raw) * 10.0,
            Self::FanOn | Self::FanOff => f64::from(raw) - 40.0,
            Self::AirCharge => f64::from(raw),
        }
    }

    pub(super) fn encode(self, value: f64) -> Result<u8, FirmwareError> {
        let spec = self.spec();
        if !self.writable() {
            return Err(FirmwareError::ReadOnly);
        }
        if !value.is_finite() {
            return Err(FirmwareError::NonFinite);
        }
        let (zero, bounds) = if self == Self::Ignition {
            (0.0, (-128.0, 127.0))
        } else {
            (-spec.min / spec.step, (0.0, 255.0))
        };
        let encoded = quantize_raw(value, spec.step, zero, bounds);
        Ok(encoded as i16 as u8)
    }
}

/// Temperature of a desired idle-speed curve node, in degrees Celsius.
pub fn idle_temperature(index: usize) -> Option<i32> {
    (index < Parameter::Idle.count()).then(|| -40 + 5 * index as i32)
}

/// A node in a rectangular table, with RPM along the columns.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TableCell {
    column: usize,
    row: usize,
    index: usize,
}

impl TableCell {
    pub fn new(column: usize, row: usize, parameter: Parameter) -> Option<Self> {
        let &[columns, rows] = parameter.spec().shape else {
            return None;
        };
        (column < columns && row < rows).then(|| Self {
            column,
            row,
            index: row * columns + column,
        })
    }

    pub fn from_index(index: usize, parameter: Parameter) -> Option<Self> {
        let &[columns, _] = parameter.spec().shape else {
            return None;
        };
        Self::new(index % columns, index / columns, parameter)
    }

    pub fn index(self) -> usize {
        self.index
    }

    pub fn column(self) -> usize {
        self.column
    }

    pub fn row(self) -> usize {
        self.row
    }
}

#[derive(Clone, Copy)]
pub enum Batch {
    Set,
    Shift,
    Percent,
}

/// Logical calibration types exposed by an editor.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CalibrationKind {
    TextList,
    CheckboxList,
    Table2D,
    Table3D,
    Number,
}
