use super::{Editor, View, parse_value};
use eframe::egui::{self, Event};
use janvar_editor::firmware::{CalibrationKind, FirmwareError, Image, MapCalibration, Parameter};
use std::ops::RangeInclusive;

#[derive(Clone, Copy, Default, PartialEq, Eq)]
pub struct Cursor {
    pub index: usize,
    pub range: Option<CellRange>,
}

impl Cursor {
    pub fn select(&mut self, index: usize, extend: bool) {
        self.range = Some(match self.range {
            Some(range) if extend => CellRange {
                anchor: range.anchor,
                end: index,
            },
            _ => CellRange::single(index),
        });
        self.index = index;
    }
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct CellRange {
    anchor: usize,
    end: usize,
}

impl CellRange {
    pub fn single(index: usize) -> Self {
        Self {
            anchor: index,
            end: index,
        }
    }

    pub fn axes(self, columns: usize) -> [RangeInclusive<usize>; 2] {
        let [x1, x2] = [self.anchor % columns, self.end % columns];
        let [y1, y2] = [self.anchor / columns, self.end / columns];
        [x1.min(x2)..=x1.max(x2), y1.min(y2)..=y1.max(y2)]
    }

    pub fn contains(self, index: usize, columns: usize) -> bool {
        let [x, y] = self.axes(columns);
        x.contains(&(index % columns)) && y.contains(&(index / columns))
    }
}

enum NumericTable {
    Known(Parameter),
    Mapped(MapCalibration),
}

impl NumericTable {
    fn dimensions(&self) -> (usize, usize) {
        let shape = match self {
            Self::Known(parameter) => parameter.spec().shape,
            Self::Mapped(calibration) => calibration.shape(),
        };
        (
            shape.first().copied().unwrap_or(1),
            shape.get(1).copied().unwrap_or(1),
        )
    }

    fn value(&self, image: &Image, index: usize) -> f64 {
        match self {
            Self::Known(parameter) => image.value(*parameter, index),
            Self::Mapped(calibration) => calibration.value(image, index),
        }
        .expect("cell in current table")
    }

    fn write(
        &self,
        image: &mut Image,
        cells: impl IntoIterator<Item = (usize, f64)>,
    ) -> Result<(), FirmwareError> {
        match self {
            Self::Known(parameter) => image.set_cells(*parameter, cells),
            Self::Mapped(calibration) => calibration.set_cells(image, cells),
        }
    }
}

impl Editor {
    pub(super) fn table_clipboard(&mut self, context: &egui::Context) {
        if self.view != View::Data
            || !context.input(|input| input.events.iter().any(is_clipboard_event))
        {
            return;
        }
        let Some(table) = self.numeric_table() else {
            return;
        };
        let events = context.input_mut(|input| {
            input
                .events
                .extract_if(.., |event| is_clipboard_event(event))
                .collect::<Vec<_>>()
        });
        for event in events {
            match event {
                Event::Copy => {
                    if let Some(range) = self.cursor.range {
                        context.copy_text(copy_range(&table, &self.image, range));
                    }
                }
                Event::Paste(text) => {
                    if let Err(error) = self.paste_cells(&table, &text) {
                        self.error = Some(error);
                        return;
                    }
                }
                _ => unreachable!("clipboard events only"),
            }
        }
    }

    fn numeric_table(&self) -> Option<NumericTable> {
        let entry = self.catalog.entries().get(self.selected)?;
        match entry.calibration_kind()? {
            CalibrationKind::Number | CalibrationKind::Table2D | CalibrationKind::Table3D => {}
            _ => return None,
        }
        match self.parameter() {
            Some(parameter) if parameter != Parameter::AirCharge => {
                Some(NumericTable::Known(parameter))
            }
            _ => entry.numeric(&self.image).ok().map(NumericTable::Mapped),
        }
    }

    fn paste_cells(&mut self, table: &NumericTable, text: &str) -> Result<(), String> {
        let (columns, rows) = table.dimensions();
        let [x, y] = self
            .cursor
            .range
            .unwrap_or(CellRange::single(0))
            .axes(columns);
        let (first_column, first_row) = (*x.start(), *y.start());
        let block = parse_block(text)?;
        let width = block[0].len();
        let height = block.len();
        if width > columns.saturating_sub(first_column) || height > rows.saturating_sub(first_row) {
            return Err("Блок не помещается в таблицу от выбранной ячейки.".into());
        }
        let start = first_row * columns + first_column;
        let cells = block.into_iter().enumerate().flat_map(|(row, values)| {
            values
                .into_iter()
                .enumerate()
                .map(move |(column, value)| (start + row * columns + column, value))
        });
        table
            .write(&mut self.image, cells)
            .map_err(|error| error.to_string())?;
        self.cursor.index = start;
        self.cursor.range = Some(CellRange {
            anchor: start,
            end: start + (height - 1) * columns + width - 1,
        });
        self.notice = None;
        self.mapped.reset();
        self.reset_input();
        Ok(())
    }
}

fn is_clipboard_event(event: &Event) -> bool {
    matches!(event, Event::Copy | Event::Paste(_))
}

fn copy_range(table: &NumericTable, image: &Image, range: CellRange) -> String {
    let (columns, _) = table.dimensions();
    let [x, y] = range.axes(columns);
    y.map(|row| {
        x.clone()
            .map(|column| table.value(image, row * columns + column).to_string())
            .collect::<Vec<_>>()
            .join("\t")
    })
    .collect::<Vec<_>>()
    .join("\n")
}

fn parse_block(text: &str) -> Result<Vec<Vec<f64>>, String> {
    let block: Vec<Vec<f64>> = text
        .lines()
        .map(|line| {
            if line.contains('\t') {
                line.split('\t').map(parse_value).collect()
            } else {
                line.split_whitespace().map(parse_value).collect()
            }
        })
        .collect::<Result<_, _>>()?;
    let width = block.first().map_or(0, Vec::len);
    if width == 0 || block.iter().any(|row| row.len() != width) {
        return Err("Вставьте прямоугольный блок чисел.".into());
    }
    Ok(block)
}
