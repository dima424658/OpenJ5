use eframe::egui::{self, Ui};
use janvar_editor::firmware::Batch;

pub fn show(ui: &mut Ui, input: &mut String, unit: &str) -> Option<Batch> {
    let mut action = None;
    egui::CollapsingHeader::new("Все точки").show(ui, |ui| {
        ui.add(
            egui::TextEdit::singleline(input)
                .hint_text("Значение")
                .desired_width(100.0),
        );
        if ui
            .button("Установить")
            .on_hover_text(format!("Задать всем точкам это значение, {unit}"))
            .clicked()
        {
            action = Some(Batch::Set);
        }
        ui.horizontal(|ui| {
            if ui.button(format!("Сдвиг {unit}")).clicked() {
                action = Some(Batch::Shift);
            }
            if ui.button("Сдвиг %").clicked() {
                action = Some(Batch::Percent);
            }
        });
    });
    action
}
