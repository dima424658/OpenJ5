use super::Editor;
use super::labels;
use super::theme::{self, ACCENT, MINT};
use eframe::egui::{RichText, Ui};
use janvar_editor::firmware::RomMode;

impl Editor {
    pub(super) fn rom_control(&mut self, ui: &mut Ui) {
        ui.add_space(3.0);
        ui.label(RichText::new("Защита калибровок").size(24.0).strong());
        ui.label(theme::caption("Диагностика P0601 · изменения в BIN"));
        ui.add_space(6.0);
        let mode = self.image.rom_mode();
        theme::card(ui, |ui| {
            let color = if mode == RomMode::Stock { MINT } else { ACCENT };
            ui.label(
                RichText::new(labels::rom_title(mode))
                    .size(21.0)
                    .strong()
                    .color(color),
            );
            match mode {
                RomMode::Stock => {
                    ui.label("Комплектация · принудительно включается младший бит.\nКонтрольная сумма · ошибка ПЗУ включается в диагностическую маску.");
                    if ui.button("Отключить как в CTP 0738").clicked() {
                        self.toggle_rom(false);
                    }
                }
                RomMode::CtpDisabled | RomMode::VendorDisabled => {
                    let description = if mode == RomMode::CtpDisabled {
                        "Комплектация · принудительная установка бита убрана.\nКонтрольная сумма · ошибка ПЗУ исключается из диагностической маски."
                    } else {
                        "Комплектация · принудительная установка бита убрана.\nКонтрольная сумма · принудительная установка бита в диагностической маске убрана."
                    };
                    ui.label(description);
                    if ui
                        .add(theme::primary("Вернуть штатные инструкции"))
                        .clicked()
                    {
                        self.toggle_rom(true);
                    }
                }
                RomMode::Unknown => {
                    ui.label(
                        "Изменение доступно для известных последовательностей инструкций L19.",
                    );
                }
                RomMode::Unsupported => {
                    ui.label(format!("Текущий профиль · {}", self.image.software_id()));
                }
            }
        });
        ui.add_space(7.0);
        ui.label(theme::caption(
            "Защита от считывания · состояние по BIN не определено",
        ));
    }

    fn toggle_rom(&mut self, enabled: bool) {
        match self.image.set_rom_control(enabled) {
            Ok(()) => self.notice = None,
            Err(error) => self.error = Some(error.to_string()),
        }
    }
}
