mod demo;
#[cfg(target_os = "macos")]
mod macos;
mod ui;

use eframe::egui;
use std::path::PathBuf;

fn main() -> eframe::Result {
    let path = std::env::args_os().nth(1).map(PathBuf::from);
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([1440.0, 900.0])
            .with_min_inner_size([900.0, 620.0])
            .with_drag_and_drop(true),
        ..Default::default()
    };
    eframe::run_native(
        "Январь 5.1 · Редактор прошивок",
        options,
        Box::new(move |context| {
            #[cfg(target_os = "macos")]
            macos::install_text_input_guard(context);
            let editor = ui::Editor::new(&context.egui_ctx, path);
            #[cfg(target_os = "macos")]
            let editor = {
                let mut editor = editor;
                editor.install_menu(&context.egui_ctx);
                editor
            };
            Ok(Box::new(editor))
        }),
    )
}
