use super::{Command, GROUPS};
use crate::ui::Editor;
use eframe::egui;
use muda::accelerator::{Accelerator, Code, Modifiers};
use muda::{Menu, MenuEvent, MenuItem, PredefinedMenuItem, Submenu};
use std::sync::mpsc::{self, Receiver};

pub(in crate::ui) struct NativeMenu {
    _bar: Menu,
    items: Vec<(Command, MenuItem)>,
    receiver: Receiver<Command>,
}

impl NativeMenu {
    pub(in crate::ui) fn install(context: &egui::Context) -> muda::Result<Self> {
        let bar = Menu::new();
        let quit = new_item(Command::Quit);
        let app = Submenu::with_items(
            "Январь 5.1",
            true,
            &[
                &PredefinedMenuItem::hide(Some("Скрыть Январь 5.1")),
                &PredefinedMenuItem::hide_others(Some("Скрыть остальные")),
                &PredefinedMenuItem::show_all(Some("Показать все")),
                &PredefinedMenuItem::separator(),
                &quit,
            ],
        )?;
        bar.append(&app)?;
        let mut items = vec![(Command::Quit, quit)];
        for &(label, commands) in GROUPS {
            let submenu = Submenu::new(label, true);
            for &command in commands {
                let item = new_item(command);
                submenu.append(&item)?;
                items.push((command, item));
            }
            bar.append(&submenu)?;
        }
        bar.init_for_nsapp();

        // Cocoa delivers selections through its own event loop. Queue the command
        // and let the editor run it on its next frame.
        let routes: Vec<_> = items
            .iter()
            .map(|(command, item)| (item.id().clone(), *command))
            .collect();
        let (sender, receiver) = mpsc::channel();
        let context = context.clone();
        MenuEvent::set_event_handler(Some(move |event: MenuEvent| {
            if let Some((_, command)) = routes.iter().find(|(id, _)| id == event.id())
                && sender.send(*command).is_ok()
            {
                context.request_repaint();
            }
        }));

        Ok(Self {
            _bar: bar,
            items,
            receiver,
        })
    }

    pub(super) fn commands(&self) -> Vec<Command> {
        self.receiver.try_iter().collect()
    }

    pub(in crate::ui) fn update(&self, editor: &Editor) {
        for (command, item) in &self.items {
            let enabled = command.enabled(editor);
            if item.is_enabled() != enabled {
                item.set_enabled(enabled);
            }
        }
    }
}

fn new_item(command: Command) -> MenuItem {
    let accelerator = match command {
        Command::OpenBin => Some(Accelerator::new(Modifiers::META, Code::KeyO)),
        Command::Save => Some(Accelerator::new(Modifiers::META, Code::KeyS)),
        Command::SaveAs => Some(Accelerator::new(
            Modifiers::META | Modifiers::SHIFT,
            Code::KeyS,
        )),
        Command::Quit => Some(Accelerator::new(Modifiers::META, Code::KeyQ)),
        // Keep text-field undo and redo in egui, which knows what has focus.
        _ => None,
    };
    MenuItem::new(command.label(), true, accelerator)
}
