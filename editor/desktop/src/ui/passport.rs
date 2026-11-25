use super::theme::{self, ACCENT};
use eframe::egui::{self, RichText, Ui};
use janvar_editor::firmware::{Image, PassportField};

#[derive(Default)]
pub struct Passport {
    input: [String; PassportField::ALL.len()],
}

impl Passport {
    pub fn sync(&mut self, image: &Image) {
        for (input, field) in self.input.iter_mut().zip(PassportField::ALL) {
            *input = image.passport(field).unwrap_or_default().into();
        }
    }

    pub fn show(&mut self, ui: &mut Ui, image: &mut Image) -> Result<(), String> {
        ui.label(RichText::new("Паспорт прошивки").size(24.0).strong());
        ui.label(theme::caption(
            "Идентификатор профиля BIN хранится в отдельном поле.",
        ));
        ui.add_space(6.0);

        egui::ScrollArea::horizontal()
            .show(ui, |ui| {
                egui::Grid::new("passport fields")
                    .spacing([12.0, 9.0])
                    .show(ui, |ui| {
                        for (input, field) in self.input.iter_mut().zip(PassportField::ALL) {
                            ui.label(title(field));
                            if let Some(current) = image.passport(field) {
                                let (address, width) = field.slot(image.profile());
                                let response = ui.add(
                                    egui::TextEdit::singleline(input)
                                        .desired_width(128.0)
                                        .font(egui::TextStyle::Monospace),
                                );
                                let has_edit = input != current;
                                let enter = response.lost_focus()
                                    && ui.input(|state| state.key_pressed(egui::Key::Enter));
                                let apply = ui
                                    .add_enabled(has_edit, egui::Button::new("Применить"))
                                    .clicked();
                                if has_edit && (apply || enter) {
                                    image
                                        .set_passport(field, input)
                                        .map_err(|error| error.to_string())?;
                                    *input = image.passport(field).unwrap_or_default().into();
                                }
                                let color = if image.passport(field) != image.saved_passport(field)
                                {
                                    ACCENT
                                } else {
                                    super::theme::MUTED
                                };
                                ui.colored_label(
                                    color,
                                    format!("0x{address:04X} · ASCII · {width}"),
                                );
                            } else {
                                ui.label(theme::caption("Поле не распознано"));
                                ui.end_row();
                                continue;
                            }
                            ui.end_row();
                        }
                        Ok(())
                    })
                    .inner
            })
            .inner
    }
}

pub fn title(field: PassportField) -> &'static str {
    match field {
        PassportField::HardwareNumber => "Номер блока",
        PassportField::EngineType => "Тип двигателя",
        PassportField::ProgrammingDate => "Дата в паспорте",
        PassportField::SoftwareIdentifier => "Идентификатор ПО в паспорте",
        PassportField::SupplierHardwareNumber => "Аппаратный номер ЭБУ от поставщика",
        PassportField::SupplierSoftwareNumber => "Программный номер ЭБУ от поставщика",
    }
}
