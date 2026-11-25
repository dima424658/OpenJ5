use super::batch;
use super::labels;
use super::table;
use super::theme::{self, MUTED};
use eframe::egui::text::{CCursor, CCursorRange};
use eframe::egui::{self, Id, RichText, Ui};
use janvar_editor::firmware::{Batch, Image, Parameter, TableCell, idle_temperature};
use janvar_editor::j5::Entry;

pub const INPUT_ID: &str = "calibration value";

pub fn focus_value(ui: &Ui, id: Id, value: &str) {
    let mut state = egui::TextEdit::load_state(ui.ctx(), id).unwrap_or_default();
    let all = CCursorRange::two(CCursor::new(0), CCursor::new(value.chars().count()));
    state.cursor.set_char_range(Some(all));
    state.store(ui.ctx(), id);
    ui.memory_mut(|memory| memory.request_focus(id));
}

pub enum ValueAction {
    Apply,
    Nudge(f64),
    Restore,
    Batch(Batch),
}

pub fn show(
    ui: &mut Ui,
    image: &Image,
    entry: &Entry,
    index: usize,
    input: &mut String,
    batch_input: &mut String,
) -> Option<ValueAction> {
    let mut action = None;
    if let Some(parameter) = entry.parameter() {
        let spec = parameter.spec();
        let info = super::reading(image, parameter, index);
        ui.label(theme::caption(match spec.shape.len() {
            0 => "Значение",
            1 => "Выбранный узел",
            _ => "Выбранная ячейка",
        }));
        if let Some(location) = location(image, parameter, index) {
            ui.label(RichText::new(location).strong());
        }
        ui.horizontal(|ui| {
            if parameter.writable() {
                let id = Id::new(INPUT_ID);
                if super::cancel_number_input(ui, id) {
                    *input = labels::format_value(parameter, info.value);
                }
                let edit = ui.add(
                    egui::TextEdit::singleline(input)
                        .id(id)
                        .font(egui::TextStyle::Monospace)
                        .desired_width(100.0),
                );
                if edit.lost_focus() && ui.input(|state| state.key_pressed(egui::Key::Enter)) {
                    action = Some(ValueAction::Apply);
                }
            } else {
                ui.label(
                    RichText::new(labels::format_value(parameter, info.value))
                        .monospace()
                        .strong(),
                );
            }
            ui.label(theme::caption(spec.unit));
        });
        if parameter.writable() {
            ui.horizontal(|ui| {
                if ui.add(theme::primary("Применить")).clicked() {
                    action = Some(ValueAction::Apply);
                }
                for (label, step) in [("−", -spec.step), ("+", spec.step)] {
                    if ui.small_button(label).clicked() {
                        action = Some(ValueAction::Nudge(step));
                    }
                }
            });
        }
        if parameter.writable()
            && !spec.shape.is_empty()
            && let Some(operation) = batch::show(ui, batch_input, spec.unit)
        {
            action = Some(ValueAction::Batch(operation));
        }
        ui.add_space(4.0);
        property(ui, "Адрес", format!("0x{:04X}", info.address));
        property(ui, "Байт", format!("0x{:02X}", info.raw));
        if info.saved_raw != info.raw {
            let saved = labels::format_value(parameter, info.saved_value);
            property(ui, "Сохранено", format!("{saved} {}", spec.unit));
            if ui.small_button("Вернуть сохранённое").clicked() {
                action = Some(ValueAction::Restore);
            }
        }
        ui.separator();
        ui.label(
            RichText::new(labels::parameter_description(parameter))
                .size(13.0)
                .color(MUTED),
        );
        ui.add_space(5.0);
        ui.separator();
    }
    property(ui, "Тип", labels::kind_title(entry.kind));
    property(ui, "ID", entry.identifier());
    property(ui, "Смещение в J5", format!("0x{:05X}", entry.offset));
    if let Some(address) = entry.address {
        let label = if address <= 0xFFFF {
            "Начало в BIN"
        } else {
            "Токен адреса"
        };
        property(ui, label, format!("0x{address:04X}"));
    }
    if !entry.units.is_empty() {
        property(ui, "Единицы в J5", &entry.units);
    }
    for (index, tag) in entry
        .tags
        .iter()
        .enumerate()
        .filter(|(_, tag)| !tag.is_empty())
    {
        property(ui, &format!("Ось {}", index + 1), tag);
    }
    action
}

fn property(ui: &mut Ui, name: &str, value: impl Into<String>) {
    ui.scope(|ui| {
        ui.spacing_mut().interact_size.y = 18.0;
        ui.horizontal(|ui| {
            ui.label(theme::caption(name));
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                ui.add(egui::Label::new(RichText::new(value).monospace().size(12.0)).truncate());
            });
        });
    });
}

pub fn location(image: &Image, parameter: Parameter, index: usize) -> Option<String> {
    match parameter {
        Parameter::Idle => idle_temperature(index).map(|temperature| format!("{temperature} °C")),
        Parameter::Ignition | Parameter::AirCharge => {
            let cell = TableCell::from_index(index, parameter)?;
            let label = table::axis_label(image, parameter, cell.row());
            let axis = if parameter == Parameter::Ignition {
                format!("{label} мг/цикл")
            } else {
                format!("строка {label}")
            };
            Some(format!("{} об/мин · {axis}", image.rpm(cell.column())?))
        }
        _ => None,
    }
}
