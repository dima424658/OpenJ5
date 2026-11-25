use super::cells::{CellRange, Cursor};
use super::curve_plot::{CurveAction, CurvePlot};
use super::labels::{format_table_value, trim_decimal};
use super::point_strip;
use super::surface::Surface;
use super::theme::{self, ACCENT, SELECTED_CELL};
use super::{inspector, scalar, table};
use eframe::egui::{self, Ui};
use janvar_editor::firmware::{Image, MapCalibration, MappedError};
use janvar_editor::j5::Entry;
use std::ops::Range;

const INPUT_ID: &str = "mapped calibration value";

pub struct MappedEditor {
    target: Option<(usize, usize)>,
    input: String,
    full_x_range: bool,
    full_y_range: bool,
}

impl Default for MappedEditor {
    fn default() -> Self {
        Self {
            target: None,
            input: String::new(),
            full_x_range: true,
            full_y_range: true,
        }
    }
}

impl MappedEditor {
    pub fn reset(&mut self) {
        self.target = None;
    }

    pub fn start_input(&mut self, context: &egui::Context, entry: usize, cell: usize) {
        self.target = Some((entry, cell));
        self.input.clear();
        context.memory_mut(|memory| memory.request_focus(egui::Id::new(INPUT_ID)));
    }

    pub fn show_value(
        &mut self,
        ui: &mut Ui,
        image: &mut Image,
        entry: &Entry,
        selected: usize,
    ) -> Result<(), String> {
        let Ok(calibration) = entry.numeric(image) else {
            return Ok(());
        };
        let selected = selected.min(calibration.count() - 1);
        let writable = calibration.writable(image);
        let unit = value_unit(entry, &calibration);
        let current = calibration.value(image, selected).expect("valid node");
        let target = (entry.offset, selected);
        if self.target != Some(target) {
            self.input = current.to_string();
            self.target = Some(target);
        }
        let apply_on_enter = ui
            .horizontal_wrapped(|ui| {
                let id = egui::Id::new(INPUT_ID);
                if super::cancel_number_input(ui, id) {
                    self.input = current.to_string();
                }
                let response = ui.add_enabled(
                    writable,
                    egui::TextEdit::singleline(&mut self.input)
                        .id(id)
                        .desired_width(110.0),
                );
                ui.label(theme::caption(unit));
                writable
                    && response.lost_focus()
                    && ui.input(|input| input.key_pressed(egui::Key::Enter))
            })
            .inner;
        if ui
            .add_enabled(writable, theme::primary("Применить"))
            .clicked()
            || apply_on_enter
        {
            let value = super::parse_value(&self.input)?;
            calibration
                .set(image, selected, value)
                .map_err(|error| error.to_string())?;
            self.input = calibration
                .value(image, selected)
                .expect("valid node")
                .to_string();
        }

        if !writable {
            let reason = if image.can_edit_map() {
                "Просмотр. Смещение находится вне области разрешённой записи."
            } else {
                "Просмотр. Для этой версии BIN схема контрольных сумм ещё не проверена."
            };
            ui.label(theme::caption(reason));
        }
        Ok(())
    }

    pub fn show(
        &mut self,
        ui: &mut Ui,
        image: &mut Image,
        entry: &Entry,
        cursor: &mut Cursor,
        show_graph: bool,
        surface: Option<&mut Surface>,
    ) -> Result<(), String> {
        let calibration = match entry.numeric(image) {
            Ok(calibration) => calibration,
            Err(MappedError::AddressNotFound) => {
                ui.colored_label(ACCENT, "Адрес не найден");
                ui.label("Выбранная калибровка не может быть отображена. Возможно, она отсутствует в этом ПО или формат прошивки нестандартный.");
                return Ok(());
            }
            Err(error) => {
                ui.colored_label(ACCENT, error.to_string());
                return Ok(());
            }
        };
        cursor.index = (cursor.index).min(calibration.count() - 1);
        let writable = calibration.writable(image);
        let unit = value_unit(entry, &calibration);
        let axis_coordinate = |axis, index| {
            entry
                .axis_value(image, axis, index)
                .filter(|value| value.is_finite())
                .map_or_else(|| format!("#{}", index + 1), format_table_value)
        };
        if let Some(&columns) = calibration.shape().first() {
            let mut coordinates = format!("X  {}", axis_coordinate(0, cursor.index % columns));
            if calibration.shape().len() == 2 {
                coordinates.push_str(&format!(
                    "  ·  Y  {}",
                    axis_coordinate(1, cursor.index / columns)
                ));
            }
            ui.label(theme::caption(coordinates));
        }
        if calibration.shape().is_empty() {
            let value = calibration.value(image, 0).expect("scalar value");
            let cell = scalar::show(ui, value, unit, cursor.range.is_some());
            if cell.clicked() {
                cursor.select(0, false);
            }
            if cell.double_clicked() && writable {
                self.input = value.to_string();
                inspector::focus_value(ui, egui::Id::new(INPUT_ID), &self.input);
            }
            return Ok(());
        }
        let mut values: Vec<_> = (0..calibration.count())
            .map(|i| calibration.value(image, i).expect("valid node"))
            .collect();
        let columns = calibration.shape()[0];
        let (min, max) = theme::value_range(&values);
        let rows = calibration.shape().get(1).copied().unwrap_or(1);
        if let Some(surface) = surface
            && let [_, _] = *calibration.shape()
        {
            if let Some(selection) = surface.show_values(
                ui,
                [columns, rows],
                &values,
                cursor.index,
                ["Столбец", "Строка", unit],
            ) {
                cursor.select(selection.index, false);
            }
            return Ok(());
        }

        if calibration.shape().len() == 1 {
            let coordinates: Option<Vec<_>> = (0..columns)
                .map(|index| entry.axis_value(image, 0, index))
                .collect();
            let physical = coordinates.filter(|axis| {
                axis.iter().all(|value| value.is_finite())
                    && axis.windows(2).all(|pair| pair[0] < pair[1])
            });
            let x_label = if physical.is_some() {
                entry.axis_label(0).unwrap_or("Ось X")
            } else {
                "Номер узла"
            };
            let x_is_physical = physical.is_some();
            let x = physical.unwrap_or_else(|| (1..=columns).map(|i| i as f64).collect());
            if show_graph && values.len() >= 2 {
                ui.horizontal_wrapped(|ui| {
                    ui.add_enabled_ui(values.len() > 2, |ui| {
                        ui.checkbox(&mut self.full_x_range, "Полная шкала X")
                            .on_hover_text(
                                "Показывает всю кривую. Снимите флажок, чтобы приблизить выбранную точку.",
                            );
                    });
                    ui.checkbox(&mut self.full_y_range, "Полная шкала Y");
                });
                let visible = visible_curve_nodes(columns, cursor.index, self.full_x_range);
                let bounds = if self.full_y_range || min == max {
                    calibration.value_bounds()
                } else {
                    let padding = (max - min) * 0.06;
                    (min - padding, max + padding)
                };
                let plot = CurvePlot {
                    x: &x[visible.clone()],
                    y: &values[visible.clone()],
                    labels: [x_label, unit],
                    bounds,
                };
                let action = ui
                    .push_id((entry.offset, cursor.index), |ui| {
                        plot.show(
                            ui,
                            cursor.index - visible.start,
                            writable.then(|| calibration.value_bounds()),
                        )
                    })
                    .inner;
                match action {
                    Some(CurveAction::Select(index)) => cursor.select(visible.start + index, false),
                    Some(CurveAction::SetValue(value)) => {
                        calibration
                            .set(image, cursor.index, value)
                            .map_err(|error| error.to_string())?;
                        values[cursor.index] =
                            calibration.value(image, cursor.index).expect("valid node");
                        self.input = values[cursor.index].to_string();
                        ui.ctx().request_repaint();
                    }
                    None => {}
                }
            } else {
                ui.label(theme::caption(x_label));
            }
            let coordinates = x.iter().map(|&coordinate| {
                let coordinate = format_table_value(coordinate);
                if !x_is_physical {
                    format!("#{coordinate}")
                } else if x_label.starts_with("Температур") {
                    format!("{coordinate} °C")
                } else {
                    coordinate
                }
            });
            let range = if show_graph {
                Some(CellRange::single(cursor.index))
            } else {
                cursor.range
            };
            if let Some((index, edit)) =
                point_strip::show(ui, entry.offset, coordinates, &values, cursor.index, range)
            {
                let extend = !show_graph && ui.input(|input| input.modifiers.shift);
                cursor.select(index, extend);
                if edit && !extend && writable {
                    self.target = Some((entry.offset, index));
                    self.input = values[index].to_string();
                    inspector::focus_value(ui, egui::Id::new(INPUT_ID), &self.input);
                }
            }
            return Ok(());
        }
        let x_labels: Vec<_> = (0..columns)
            .map(|index| axis_coordinate(0, index))
            .collect();
        let y_labels: Vec<_> = (0..rows).map(|index| axis_coordinate(1, index)).collect();
        let column_width = table::axis_width(ui, &x_labels);
        let row_header_width = table::axis_width(ui, &y_labels);
        let axes = cursor.range.map(|range| range.axes(columns));
        let range = cursor.range;
        ui.horizontal_wrapped(|ui| {
            for (axis, name) in ["X", "Y"].into_iter().enumerate() {
                if let Some(label) = entry.axis_label(axis) {
                    ui.label(theme::caption(format!("{name} · {label}")));
                }
            }
        });
        egui::ScrollArea::both()
            .id_salt(entry.offset)
            .show(ui, |ui| {
                egui::Grid::new("mapped values")
                    .spacing([2.0, 2.0])
                    .show(ui, |ui| {
                        table::axis_cell(
                            ui,
                            table::axis_text("Y / X", false),
                            false,
                            row_header_width,
                        );
                        for (column, label) in x_labels.iter().enumerate() {
                            let active = axes.as_ref().is_some_and(|[x, _]| x.contains(&column));
                            table::axis_cell(
                                ui,
                                table::axis_text(label, active),
                                active,
                                column_width,
                            );
                        }
                        ui.end_row();
                        for (row, label) in y_labels.iter().enumerate() {
                            let active = axes.as_ref().is_some_and(|[_, y]| y.contains(&row));
                            table::axis_cell(
                                ui,
                                table::axis_text(label, active),
                                active,
                                row_header_width,
                            );
                            for column in 0..columns {
                                let index = row * columns + column;
                                let value = values[index];
                                let fill =
                                    if range.is_some_and(|range| range.contains(index, columns)) {
                                        SELECTED_CELL
                                    } else {
                                        theme::heat(theme::normalize(value, min, max))
                                    };
                                let response = table::number_cell(ui, value, fill, column_width);
                                if response.clicked() {
                                    let extend = ui.input(|input| input.modifiers.shift);
                                    cursor.select(index, extend);
                                    if response.double_clicked() && !extend && writable {
                                        self.target = Some((entry.offset, index));
                                        self.input = value.to_string();
                                        inspector::focus_value(
                                            ui,
                                            egui::Id::new(INPUT_ID),
                                            &self.input,
                                        );
                                    }
                                }
                            }
                            ui.end_row();
                        }
                    });
            });
        Ok(())
    }
}

fn value_unit<'a>(entry: &'a Entry, calibration: &MapCalibration) -> &'a str {
    if calibration.uses_physical_units() {
        &entry.units
    } else {
        "raw"
    }
}

fn visible_curve_nodes(count: usize, selected: usize, full_scale: bool) -> Range<usize> {
    if full_scale {
        return 0..count;
    }
    let width = count.div_ceil(2).max(2);
    let start = selected.saturating_sub(width / 2).min(count - width);
    start..start + width
}

pub fn format_value(value: f64) -> String {
    let number = format!("{value:.6}");
    trim_decimal(&number).to_owned()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn zoomed_curve_keeps_the_selected_node_visible_at_both_ends() {
        assert_eq!(visible_curve_nodes(39, 25, true), 0..39);
        assert_eq!(visible_curve_nodes(39, 0, false), 0..20);
        assert_eq!(visible_curve_nodes(39, 15, false), 5..25);
        assert_eq!(visible_curve_nodes(39, 38, false), 19..39);
        assert_eq!(visible_curve_nodes(2, 1, false), 0..2);
    }

    #[test]
    fn mapped_cells_have_equal_width_for_different_number_lengths() {
        let context = egui::Context::default();
        let _ = context.run(egui::RawInput::default(), |ctx| {
            egui::CentralPanel::default().show(ctx, |ui| {
                ui.horizontal(|ui| {
                    let widths = [0.0, 0.6015625, -0.0001, 65_535.0, -123_456.75]
                        .map(|value| table::number_cell(ui, value, theme::BG, 64.0).rect.width());
                    assert_eq!(widths, [64.0; 5]);
                });
            });
        });
    }

    #[test]
    fn mapped_table_places_axis_values_in_the_header_row_and_column() {
        use crate::demo;

        let context = egui::Context::default();
        let mut image = Image::new(demo::SAMPLE.to_vec()).unwrap();
        let (catalog, _) = demo::reference(image.profile()).unwrap();
        let entry = catalog
            .entries()
            .iter()
            .find(|entry| entry.address == Some(0x83BA))
            .unwrap();
        let x = format_table_value(entry.axis_value(&image, 0, 0).unwrap());
        let y = format_table_value(entry.axis_value(&image, 1, 0).unwrap());
        let mut editor = MappedEditor::default();
        let mut cursor = Cursor::default();
        let output = context.run(egui::RawInput::default(), |ctx| {
            egui::CentralPanel::default().show(ctx, |ui| {
                editor
                    .show(ui, &mut image, entry, &mut cursor, false, None)
                    .unwrap();
            });
        });
        let position = |label: &str| {
            output
                .shapes
                .iter()
                .find_map(|shape| match &shape.shape {
                    egui::Shape::Text(text) if text.galley.text() == label => {
                        Some(text.pos + text.galley.size() * 0.5)
                    }
                    _ => None,
                })
                .unwrap()
        };
        let corner = position("Y / X");
        let x = position(&x);
        let y = position(&y);
        assert!(x.x > corner.x);
        assert!((x.y - corner.y).abs() < 1.0);
        assert!((y.x - corner.x).abs() < 1.0);
        assert!(y.y > corner.y);
    }
}
