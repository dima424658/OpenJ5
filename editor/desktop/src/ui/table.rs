use super::Selection;
use super::cells::Cursor;
use super::labels;
use super::theme::{self, BG, CARD, LINE, MUTED, PANEL, SELECTED_CELL, TEXT};
use eframe::egui::{
    self, Align2, Color32, FontId, Pos2, Rect, RichText, Sense, Stroke, StrokeKind, Ui, pos2, vec2,
};
use janvar_editor::firmware::{Image, Parameter, TableCell};

pub fn number_cell(ui: &mut Ui, value: f64, fill: Color32, width: f32) -> egui::Response {
    let text = RichText::new(labels::format_table_value(value))
        .monospace()
        .color(BG);
    ui.add_sized([width, 25.0], egui::Button::new(text).fill(fill).truncate())
        .on_hover_text(value.to_string())
}

pub fn axis_text(label: impl Into<String>, active: bool) -> RichText {
    RichText::new(label)
        .monospace()
        .color(if active { SELECTED_CELL } else { MUTED })
}

fn axis_fill(active: bool) -> Color32 {
    if active { LINE } else { PANEL }
}

pub fn axis_cell(ui: &mut Ui, label: impl Into<egui::WidgetText>, active: bool, width: f32) {
    ui.add_sized(
        [width, 25.0],
        egui::Button::new(label)
            .sense(Sense::hover())
            .fill(axis_fill(active))
            .stroke(Stroke::new(1.0_f32, LINE))
            .truncate(),
    );
}

pub fn axis_width(ui: &Ui, labels: &[String]) -> f32 {
    let font = egui::TextStyle::Monospace.resolve(ui.style());
    let padding = 2.0 * ui.spacing().button_padding.x;
    labels
        .iter()
        .map(|label| {
            ui.painter()
                .layout_no_wrap(label.clone(), font.clone(), MUTED)
                .size()
                .x
                + padding
        })
        .fold(64.0, f32::max)
        .ceil()
}

pub fn axis_label(image: &Image, parameter: Parameter, row: usize) -> String {
    if parameter == Parameter::Ignition {
        image
            .load_axis(row)
            .map_or_else(|| format!("#{}", row + 1), |load| load.to_string())
    } else {
        (row + 1).to_string()
    }
}

pub fn show(ui: &mut Ui, image: &Image, parameter: Parameter, cursor: Cursor) -> Option<Selection> {
    let values: Vec<_> = (0..parameter.count())
        .map(|index| super::current(image, parameter, index))
        .collect();
    let (min, max) = theme::value_range(&values);
    let [cols, rows] = parameter.spec().shape else {
        return None;
    };
    let (cols, rows) = (*cols, *rows);
    let axes = cursor.range.map(|range| range.axes(cols));
    ui.horizontal(|ui| {
        ui.label(theme::caption("об/мин из BIN"));
        ui.separator();
        ui.label(theme::caption(if parameter == Parameter::Ignition {
            "мг/цикл"
        } else {
            "индекс"
        }));
    });
    ui.add_space(4.0);

    let mut selection = None;
    egui::ScrollArea::both()
        .id_salt(parameter.spec().address)
        .auto_shrink([true, true])
        .max_height((ui.available_height() - 55.0).max(120.0))
        .show(ui, |ui| {
            let axis = 49.0;
            let cell = ((ui.available_width() - axis) / cols as f32).max(41.0);
            let row_height = ((ui.available_height() - 28.0) / (rows + 1) as f32).clamp(24.0, 31.0);
            let (rect, painter) = ui.allocate_painter(
                vec2(axis + cell * cols as f32, row_height * (rows + 1) as f32),
                Sense::hover(),
            );
            let origin = rect.rect.min;
            painter.rect_filled(rect.rect, 5, CARD);
            let number = FontId::monospace(12.0);
            let header = |slot: Rect, label: String, active: bool| {
                painter.rect_filled(slot.shrink(1.0), 2, axis_fill(active));
                painter.text(
                    slot.center(),
                    Align2::CENTER_CENTER,
                    label,
                    number.clone(),
                    if active { SELECTED_CELL } else { MUTED },
                );
            };
            header(
                Rect::from_min_size(origin, vec2(axis, row_height)),
                "Y / X".into(),
                false,
            );
            for column in 0..cols {
                let left = origin.x + axis + column as f32 * cell;
                header(
                    Rect::from_min_size(pos2(left, origin.y), vec2(cell, row_height)),
                    image.rpm(column).expect("RPM column").to_string(),
                    axes.as_ref().is_some_and(|[x, _]| x.contains(&column)),
                );
            }
            for row in 0..rows {
                let top = origin.y + (row + 1) as f32 * row_height;
                header(
                    Rect::from_min_size(pos2(origin.x, top), vec2(axis, row_height)),
                    axis_label(image, parameter, row),
                    axes.as_ref().is_some_and(|[_, y]| y.contains(&row)),
                );
                for column in 0..cols {
                    let index = TableCell::new(column, row, parameter)
                        .expect("visible cell")
                        .index();
                    let left = origin.x + axis + column as f32 * cell;
                    let slot = Rect::from_min_size(pos2(left, top), vec2(cell, row_height));
                    let response = ui.interact(slot, ui.id().with(index), Sense::click());
                    let active = cursor
                        .range
                        .is_some_and(|range| range.contains(index, cols));
                    let fill = if active {
                        SELECTED_CELL
                    } else {
                        theme::heat(theme::normalize(values[index], min, max))
                    };
                    painter.rect_filled(slot.shrink(1.0), 2, fill);
                    let focused = active && index == cursor.index;
                    if focused || response.hovered() {
                        painter.rect_stroke(
                            slot.shrink(0.5),
                            2,
                            Stroke::new(2.0_f32, if focused { TEXT } else { BG }),
                            StrokeKind::Inside,
                        );
                    } else if axes
                        .as_ref()
                        .is_some_and(|[x, y]| x.contains(&column) || y.contains(&row))
                    {
                        painter.rect_stroke(
                            slot.shrink(1.0),
                            2,
                            Stroke::new(1.0_f32, LINE),
                            StrokeKind::Inside,
                        );
                    }
                    let info = super::reading(image, parameter, index);
                    if info.raw != info.saved_raw {
                        painter.circle_filled(slot.right_top() + vec2(-5.0, 5.0), 2.2, BG);
                    }
                    painter.text(
                        slot.center(),
                        Align2::CENTER_CENTER,
                        labels::format_value(parameter, values[index]),
                        number.clone(),
                        BG,
                    );
                    let double_click = response.double_clicked();
                    if response.clicked() || double_click {
                        selection = Some(Selection {
                            index,
                            edit: double_click,
                        });
                    }
                    response.on_hover_text(format!("0x{:04X} · {:02X}", info.address, info.raw));
                }
            }
        });

    ui.add_space(6.0);
    legend(ui, parameter, min, max);
    selection
}

pub fn legend(ui: &mut Ui, parameter: Parameter, min: f64, max: f64) {
    ui.horizontal(|ui| {
        ui.label(theme::caption(labels::format_value(parameter, min)));
        let (response, painter) = ui.allocate_painter(vec2(124.0, 10.0), Sense::hover());
        for index in 0..32 {
            let left = response.rect.left() + response.rect.width() * index as f32 / 32.0;
            let piece = Rect::from_min_size(
                Pos2::new(left, response.rect.top()),
                vec2(response.rect.width() / 32.0 + 0.1, 10.0),
            );
            painter.rect_filled(piece, 0, theme::heat(index as f32 / 31.0));
        }
        ui.label(theme::caption(format!(
            "{} {}",
            labels::format_value(parameter, max),
            parameter.spec().unit
        )));
    });
}
