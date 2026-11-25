use super::theme::{self, ACCENT};
use eframe::egui::{self, RichText, Ui};
use janvar_editor::firmware::Image;
use janvar_editor::j5::Flag;

pub fn show(ui: &mut Ui, image: &mut Image, flags: &[Flag]) -> Result<(), String> {
    ui.label(theme::caption(
        "Отмеченный флажок соответствует установленному биту в BIN.",
    ));
    ui.add_space(4.0);
    egui::ScrollArea::vertical()
        .id_salt("checkbox list")
        .auto_shrink([false, true])
        .show(ui, |ui| {
            let mut section = "";
            for flag in flags {
                let Some(mut checked) = image.flag(flag.address, flag.bit) else {
                    continue;
                };
                if flag.section != section {
                    ui.add_space(4.0);
                    ui.label(RichText::new(&flag.section).strong());
                    section = &flag.section;
                }
                let mut label = RichText::new(&flag.title);
                if Some(checked) != image.saved_flag(flag.address, flag.bit) {
                    label = label.color(ACCENT);
                }
                if ui
                    .checkbox(&mut checked, label)
                    .on_hover_text(format!("0x{:04X} · бит {}", flag.address, flag.bit))
                    .changed()
                {
                    image
                        .set_flag(flag.address, flag.bit, checked)
                        .map_err(|error| error.to_string())?;
                }
            }
            Ok(())
        })
        .inner
}
