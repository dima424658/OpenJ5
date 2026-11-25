use super::labels;
use super::theme::{self, ACCENT, MINT, PANEL};
use super::{Editor, View};
use eframe::egui::{self, Margin, RichText, Ui};
use janvar_editor::firmware::{CalibrationKind, Image};

pub fn footer(editor: &Editor, context: &egui::Context) {
    egui::TopBottomPanel::bottom("status bar")
        .frame(theme::panel(PANEL, Margin::symmetric(14, 6)))
        .show(context, |ui| {
            ui.horizontal_wrapped(|ui| {
                let valid = editor.image.checksums().iter().all(|sum| sum.valid());
                let (status, color) = if valid {
                    ("Суммы сходятся", MINT)
                } else {
                    ("Суммы будут обновлены при сохранении", ACCENT)
                };
                theme::status_dot(ui, color);
                let status = ui
                    .link(RichText::new(status).size(12.0).color(color))
                    .on_hover_text("Показать контрольные суммы");
                egui::Popup::from_toggle_button_response(&status)
                    .align(egui::RectAlign::TOP_START)
                    .show(|ui| checksums(ui, &editor.image));

                ui.separator();
                ui.label(theme::caption(format!("BIN · {}", editor.file_name())));
                ui.separator();
                if editor.catalog.entries().is_empty() {
                    ui.label(theme::caption(&editor.catalog_source));
                } else {
                    ui.label(theme::caption(format!("J5 · {}", editor.catalog_source)));
                    ui.separator();
                    ui.label(theme::caption(format!(
                        "CTP 3.21.{}",
                        editor.catalog.build()
                    )));
                }
                if let Some(message) = &editor.notice {
                    ui.separator();
                    ui.label(theme::caption(message));
                }
            });
        });
}

fn checksums(ui: &mut Ui, image: &Image) {
    ui.heading("Контрольные суммы");
    egui::Grid::new("checksum values")
        .striped(true)
        .spacing([12.0, 5.0])
        .show(ui, |ui| {
            ui.label(theme::caption("Проверка"));
            ui.label(theme::caption("В BIN"));
            ui.label(theme::caption("Расчёт"));
            ui.end_row();

            for sum in image.checksums() {
                let color = if sum.valid() { MINT } else { ACCENT };
                ui.label(labels::checksum_title(sum.kind));
                ui.colored_label(
                    color,
                    RichText::new(format!("{:04X}", sum.stored)).monospace(),
                );
                ui.colored_label(
                    color,
                    RichText::new(format!("{:04X}", sum.calculated)).monospace(),
                );
                ui.end_row();
            }
        });
}

pub fn tabs(editor: &mut Editor, ui: &mut Ui) {
    let kind = editor.calibration_kind();
    ui.horizontal_wrapped(|ui| {
        let label = match kind {
            Some(CalibrationKind::Table3D | CalibrationKind::Table2D | CalibrationKind::Number) => {
                "Таблица"
            }
            Some(CalibrationKind::CheckboxList) => "Флажки",
            _ => "Калибровка",
        };
        if editor.view != View::Passport {
            ui.selectable_value(&mut editor.view, View::Data, label);
            if kind == Some(CalibrationKind::Table2D) {
                ui.selectable_value(&mut editor.view, View::Graph, "График 2D");
            }
            if kind == Some(CalibrationKind::Table3D) {
                ui.selectable_value(&mut editor.view, View::Surface, "Поверхность 3D");
            }
        }
        let changes = editor.change_count();
        ui.selectable_value(
            &mut editor.view,
            View::Changes,
            format!("Изменения · {changes}"),
        );
    });
    ui.separator();
}
