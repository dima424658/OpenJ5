#[cfg(target_os = "macos")]
mod native;

#[cfg(target_os = "macos")]
pub(super) use native::NativeMenu;

use super::{Editor, FileAction, View, theme};
use eframe::egui::{self, Key, KeyboardShortcut, Margin, Modifiers};
use janvar_editor::firmware::CalibrationKind;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Command {
    OpenBin,
    OpenMap,
    Save,
    SaveAs,
    Quit,
    Undo,
    Redo,
    Calibrations,
    Table2D,
    Graph2D,
    ToggleView,
    Passport,
    Changes,
    Rom,
}

const FILE: &[Command] = &[
    Command::OpenBin,
    Command::OpenMap,
    Command::Save,
    Command::SaveAs,
];
const EDIT: &[Command] = &[Command::Undo, Command::Redo];
const VIEW: &[Command] = &[
    Command::Calibrations,
    Command::Table2D,
    Command::Graph2D,
    Command::ToggleView,
    Command::Passport,
    Command::Changes,
    Command::Rom,
];
const GROUPS: &[(&str, &[Command])] = &[("Файл", FILE), ("Правка", EDIT), ("Вид", VIEW)];

impl Command {
    fn label(self) -> &'static str {
        match self {
            Self::OpenBin => "Открыть BIN…",
            Self::OpenMap => "Открыть J5…",
            Self::Save => "Сохранить",
            Self::SaveAs => "Сохранить как…",
            Self::Quit => "Выйти",
            Self::Undo => "Отменить правку",
            Self::Redo => "Повторить правку",
            Self::Calibrations => "Калибровки",
            Self::Table2D => "Таблица",
            Self::Graph2D => "График 2D",
            Self::ToggleView => "Переключить вид",
            Self::Passport => "Паспорт",
            Self::Changes => "Изменения",
            Self::Rom => "Защита калибровок",
        }
    }

    fn shortcut(self) -> Option<KeyboardShortcut> {
        let (modifiers, key) = match self {
            Self::OpenBin => (Modifiers::COMMAND, Key::O),
            Self::Save => (Modifiers::COMMAND, Key::S),
            Self::SaveAs => (Modifiers::COMMAND | Modifiers::SHIFT, Key::S),
            Self::Quit => (Modifiers::COMMAND, Key::Q),
            Self::Undo => (Modifiers::COMMAND, Key::Z),
            Self::Redo => (Modifiers::COMMAND | Modifiers::SHIFT, Key::Z),
            Self::ToggleView => (Modifiers::NONE, Key::D),
            _ => return None,
        };
        Some(KeyboardShortcut::new(modifiers, key))
    }

    fn enabled(self, editor: &Editor) -> bool {
        if editor.confirm.is_some() || editor.error.is_some() {
            return false;
        }
        match self {
            Self::Save | Self::SaveAs => editor.image.can_edit_map(),
            Self::Undo => editor.image.can_undo(),
            Self::Redo => editor.image.can_redo(),
            Self::ToggleView => alternate_view(editor).is_some(),
            Self::Table2D | Self::Graph2D => {
                editor.calibration_kind() == Some(CalibrationKind::Table2D)
            }
            _ => true,
        }
    }

    fn run(self, editor: &mut Editor) {
        if !self.enabled(editor) {
            return;
        }
        match self {
            Self::OpenBin => editor.request(FileAction::OpenBin),
            Self::OpenMap => editor.request(FileAction::OpenMap),
            Self::Save => editor.request(FileAction::Save),
            Self::SaveAs => editor.request(FileAction::SaveAs),
            Self::Quit => editor.request(FileAction::Close),
            Self::Undo => editor.undo(),
            Self::Redo => editor.redo(),
            Self::Calibrations | Self::Table2D => editor.view = View::Data,
            Self::Graph2D => editor.view = View::Graph,
            Self::ToggleView => {
                if let Some(view) = alternate_view(editor) {
                    editor.view = view;
                }
            }
            Self::Passport => editor.view = View::Passport,
            Self::Changes => editor.view = View::Changes,
            Self::Rom => editor.view = View::Rom,
        }
    }
}

fn alternate_view(editor: &Editor) -> Option<View> {
    match (editor.calibration_kind(), editor.view) {
        (Some(CalibrationKind::Table2D), View::Data) => Some(View::Graph),
        (Some(CalibrationKind::Table3D), View::Data) => Some(View::Surface),
        (Some(CalibrationKind::Table2D), View::Graph)
        | (Some(CalibrationKind::Table3D), View::Surface) => Some(View::Data),
        _ => None,
    }
}

pub(super) fn has_native_menu(editor: &Editor) -> bool {
    #[cfg(target_os = "macos")]
    {
        editor.native_menu.is_some()
    }
    #[cfg(not(target_os = "macos"))]
    {
        let _ = editor;
        false
    }
}

pub(super) fn shortcut(editor: &mut Editor, state: &mut egui::InputState, editing: bool) -> bool {
    if !editing && Command::ToggleView.enabled(editor) && consume_view_key(state) {
        Command::ToggleView.run(editor);
        return true;
    }
    // macOS handles file and app shortcuts through the native menu when installed.
    let commands: &[Command] = match (has_native_menu(editor), editing) {
        (true, true) => &[],
        (true, false) => &[Command::Redo, Command::Undo],
        (false, true) => &[
            Command::SaveAs,
            Command::OpenBin,
            Command::Save,
            Command::Quit,
        ],
        (false, false) => &[
            Command::SaveAs,
            Command::OpenBin,
            Command::Save,
            Command::Quit,
            Command::Redo,
            Command::Undo,
        ],
    };
    for &command in commands {
        if let Some(shortcut) = command.shortcut()
            && state.consume_shortcut(&shortcut)
        {
            command.run(editor);
            return true;
        }
    }
    false
}

fn consume_view_key(state: &mut egui::InputState) -> bool {
    let mut pressed = false;
    state.events.retain(|event| {
        if let egui::Event::Key {
            key: Key::D,
            modifiers: Modifiers::NONE,
            pressed: true,
            repeat,
            ..
        } = event
        {
            pressed |= !repeat;
            false
        } else {
            true
        }
    });
    pressed
}

pub(super) fn show(editor: &mut Editor, context: &egui::Context) {
    #[cfg(target_os = "macos")]
    if let Some(native) = &editor.native_menu {
        let commands = native.commands();
        for command in commands {
            command.run(editor);
        }
        return;
    }

    egui::TopBottomPanel::top("application menu")
        .frame(theme::panel(theme::PANEL, Margin::symmetric(12, 3)))
        .show(context, |ui| {
            egui::MenuBar::new().ui(ui, |ui| {
                for &(label, commands) in GROUPS {
                    ui.menu_button(label, |ui| {
                        for &command in commands {
                            item(ui, editor, command);
                        }
                        if label == "Файл" {
                            ui.separator();
                            item(ui, editor, Command::Quit);
                        }
                    });
                }
            });
        });
}

fn item(ui: &mut egui::Ui, editor: &mut Editor, command: Command) {
    let mut button = egui::Button::new(command.label());
    if let Some(shortcut) = command.shortcut() {
        button = button.shortcut_text(ui.ctx().format_shortcut(&shortcut));
    }
    if ui.add_enabled(command.enabled(editor), button).clicked() {
        command.run(editor);
        ui.close();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use janvar_editor::firmware::Parameter;

    fn send_key(
        editor: &mut Editor,
        context: &egui::Context,
        key: Key,
        modifiers: Modifiers,
        pressed: bool,
    ) {
        let input = egui::RawInput {
            modifiers,
            events: vec![egui::Event::Key {
                key,
                physical_key: Some(key),
                pressed,
                repeat: false,
                modifiers,
            }],
            ..Default::default()
        };
        let _ = context.run(input, |ctx| editor.input_events(ctx));
    }

    #[test]
    fn d_switches_table_views_and_ignores_repeat_and_text_focus() {
        let context = egui::Context::default();
        let mut editor = Editor::new(&context, None);
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, true);
        assert!(editor.view == View::Surface);
        assert_eq!(editor.cursor.index, 17);
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, true);
        assert!(editor.view == View::Surface);
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, false);
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, true);
        assert!(editor.view == View::Data);

        editor.select_parameter(Parameter::Idle, 24);
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, false);
        send_key(&mut editor, &context, Key::D, Modifiers::SHIFT, true);
        assert!(editor.view == View::Data);
        send_key(&mut editor, &context, Key::D, Modifiers::SHIFT, false);
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, true);
        assert!(editor.view == View::Graph);
        assert_eq!(editor.cursor.index, 24);

        send_key(&mut editor, &context, Key::D, Modifiers::NONE, false);
        let _ = context.run(egui::RawInput::default(), |ctx| editor.draw(ctx));
        context.memory_mut(|memory| {
            memory.request_focus(egui::Id::new(crate::ui::inspector::INPUT_ID));
        });
        send_key(&mut editor, &context, Key::D, Modifiers::NONE, true);
        assert!(editor.view == View::Graph);
    }

    #[test]
    fn undo_redo_and_save_as_shortcuts_dispatch_their_commands() {
        let context = egui::Context::default();
        let mut editor = Editor::new(&context, None);
        editor.image.set(Parameter::FanOn, 0, 42.0).unwrap();
        context.memory_mut(|memory| memory.request_focus(egui::Id::new("table cell")));

        send_key(&mut editor, &context, Key::Z, Modifiers::COMMAND, true);
        assert!(!editor.dirty());
        send_key(&mut editor, &context, Key::Z, Modifiers::COMMAND, false);
        send_key(
            &mut editor,
            &context,
            Key::Z,
            Modifiers::COMMAND | Modifiers::SHIFT,
            true,
        );
        assert!(editor.dirty());

        send_key(
            &mut editor,
            &context,
            Key::S,
            Modifiers::COMMAND | Modifiers::SHIFT,
            true,
        );
        assert!(matches!(editor.pending, Some(FileAction::SaveAs)));
    }

    #[test]
    fn view_menu_switches_curve_views_without_moving_the_selected_node() {
        let context = egui::Context::default();
        let mut editor = Editor::new(&context, None);
        assert!(!Command::Graph2D.enabled(&editor));

        editor.select_parameter(Parameter::Idle, 24);
        assert!(editor.view == View::Data);
        Command::Graph2D.run(&mut editor);
        assert!(editor.view == View::Graph);
        assert_eq!(editor.cursor.index, 24);
        Command::Table2D.run(&mut editor);
        assert!(editor.view == View::Data);
        assert_eq!(editor.cursor.index, 24);
    }

    #[test]
    fn save_overwrites_the_open_bin_and_updates_the_saved_baseline() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("current.bin");
        std::fs::write(&path, crate::demo::SAMPLE).unwrap();
        let context = egui::Context::default();
        let mut editor = Editor::new(&context, Some(path.clone()));
        editor.image.set(Parameter::FanOn, 0, 42.0).unwrap();

        send_key(&mut editor, &context, Key::S, Modifiers::COMMAND, true);
        let _ = context.run(egui::RawInput::default(), |ctx| editor.draw(ctx));

        let saved = janvar_editor::firmware::open(&path).unwrap();
        assert_eq!(saved.value(Parameter::FanOn, 0), Some(42.0));
        assert!(saved.checksums().iter().all(|sum| sum.valid()));
        assert!(!editor.dirty());
        assert!(editor.error.is_none());
        assert_eq!(std::fs::read_dir(directory.path()).unwrap().count(), 1);
    }

    #[test]
    fn quitting_from_menu_uses_the_unsaved_changes_confirmation() {
        let context = egui::Context::default();
        let mut editor = Editor::new(&context, None);
        editor.image.set(Parameter::FanOn, 0, 42.0).unwrap();
        Command::Quit.run(&mut editor);
        let _ = context.run(egui::RawInput::default(), |ctx| editor.draw(ctx));
        assert!(matches!(editor.confirm, Some(FileAction::Close)));
        assert!(!editor.allow_close);
        assert!(editor.dirty());
    }
}
