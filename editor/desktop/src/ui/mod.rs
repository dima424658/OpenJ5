mod batch;
mod cells;
mod chrome;
mod curve;
mod curve_plot;
mod files;
mod flags;
mod inspector;
mod labels;
mod mapped;
mod menu;
mod navigation;
mod passport;
mod point_strip;
mod programming;
mod protection;
mod scalar;
mod surface;
mod table;
mod theme;

use self::inspector::ValueAction;
use self::navigation::{Navigation, Target};
use self::surface::Surface;
use self::theme::{ACCENT, BG, MUTED, PANEL};
use crate::demo;
use eframe::egui::{self, Key, Margin, Modifiers, RichText};
use janvar_editor::firmware::{CalibrationKind, Image, Parameter, PassportField, Reading};
use janvar_editor::j5::{Catalog, Flag};
use std::collections::BTreeSet;
use std::path::PathBuf;

#[derive(Clone, Copy, PartialEq, Eq)]
struct Selection {
    index: usize,
    edit: bool,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum View {
    Data,
    Graph,
    Surface,
    Changes,
    Rom,
    Passport,
}

impl View {
    fn is_calibration(self) -> bool {
        matches!(self, Self::Data | Self::Graph | Self::Surface)
    }
}

enum FileAction {
    OpenBin,
    OpenMap,
    Load(PathBuf),
    Save,
    SaveAs,
    Close,
}

pub struct Editor {
    #[cfg(target_os = "macos")]
    native_menu: Option<menu::NativeMenu>,
    image: Image,
    catalog: Catalog,
    reference_0738: Option<Catalog>,
    catalog_source: String,
    navigation: Navigation,
    mapped: mapped::MappedEditor,
    passport: passport::Passport,
    surface: Surface,
    selected: usize,
    cursor: cells::Cursor,
    text_edit_was_focused: bool,
    view: View,
    input: String,
    batch_input: String,
    source: Option<PathBuf>,
    destination: Option<PathBuf>,
    pending: Option<FileAction>,
    confirm: Option<FileAction>,
    allow_close: bool,
    notice: Option<String>,
    error: Option<String>,
    programming: programming::Programming,
}

impl Editor {
    pub fn new(context: &egui::Context, path: Option<PathBuf>) -> Self {
        theme::install(context);
        let image = Image::new(demo::SAMPLE.to_vec()).expect("bundled firmware");
        let (catalog, name) = demo::reference(image.profile()).expect("bundled sample catalog");
        let mut passport = passport::Passport::default();
        passport.sync(&image);
        let mut editor = Self {
            #[cfg(target_os = "macos")]
            native_menu: None,
            image,
            passport,
            navigation: Navigation::new(&catalog),
            catalog,
            reference_0738: None,
            mapped: mapped::MappedEditor::default(),
            catalog_source: name.into(),
            surface: Surface::default(),
            selected: 0,
            cursor: cells::Cursor::default(),
            text_edit_was_focused: false,
            view: View::Data,
            input: String::new(),
            batch_input: String::new(),
            source: None,
            destination: None,
            pending: None,
            confirm: None,
            allow_close: false,
            notice: None,
            error: None,
            programming: programming::Programming::default(),
        };
        editor.select_parameter(Parameter::Ignition, 17);
        if let Some(path) = path {
            editor.error = editor.open_bin(path).err();
        }
        editor
    }

    #[cfg(target_os = "macos")]
    pub fn install_menu(&mut self, context: &egui::Context) {
        match menu::NativeMenu::install(context) {
            Ok(menu) => {
                menu.update(self);
                self.native_menu = Some(menu);
            }
            Err(error) => eprintln!("Не удалось создать системное меню: {error}"),
        }
    }

    fn set_catalog(&mut self, catalog: Catalog, name: String) {
        self.navigation = Navigation::new(&catalog);
        self.catalog = catalog;
        self.catalog_source = name;
        let entry = self.catalog.find(Parameter::Ignition).unwrap_or(0);
        self.select(entry);
    }

    fn parameter(&self) -> Option<Parameter> {
        self.catalog.entries().get(self.selected).and_then(|entry| {
            self.image
                .can_edit_map()
                .then(|| entry.parameter())
                .flatten()
        })
    }

    fn calibration_kind(&self) -> Option<CalibrationKind> {
        self.catalog
            .entries()
            .get(self.selected)
            .and_then(|entry| entry.calibration_kind())
    }

    fn select(&mut self, index: usize) {
        self.selected = index;
        self.cursor.range = None;
        self.cursor.index = match self.parameter() {
            Some(Parameter::Ignition) => 17,
            Some(Parameter::Idle) => 24,
            _ => 0,
        };
        self.view = View::Data;
        self.batch_input.clear();
        self.mapped.reset();
        self.reset_input();
    }

    fn select_parameter(&mut self, parameter: Parameter, cell: usize) {
        if let Some(index) = self.catalog.find(parameter) {
            self.select(index);
            self.cursor.index = cell;
            self.reset_input();
        }
    }

    fn reset_input(&mut self) {
        self.input = self.parameter().map_or_else(String::new, |param| {
            labels::format_value(param, current(&self.image, param, self.cursor.index))
        });
    }

    fn apply(&mut self, action: ValueAction) {
        let Some(parameter) = self.parameter() else {
            return;
        };
        let result = match action {
            ValueAction::Apply => parse_value(&self.input).and_then(|value| {
                self.image
                    .set(parameter, self.cursor.index, value)
                    .map_err(|error| error.to_string())
            }),
            ValueAction::Nudge(delta) => {
                let value = current(&self.image, parameter, self.cursor.index) + delta;
                self.image
                    .set(parameter, self.cursor.index, value)
                    .map_err(|error| error.to_string())
            }
            ValueAction::Restore => {
                let value = reading(&self.image, parameter, self.cursor.index).saved_value;
                self.image
                    .set(parameter, self.cursor.index, value)
                    .map_err(|error| error.to_string())
            }
            ValueAction::Batch(operation) => parse_value(&self.batch_input).and_then(|value| {
                self.image
                    .apply_batch(parameter, operation, value)
                    .map_err(|error| error.to_string())
            }),
        };
        match result {
            Ok(()) => {
                self.notice = None;
                self.reset_input();
            }
            Err(error) => self.error = Some(error),
        }
    }

    fn undo(&mut self) {
        if self.image.undo() {
            self.passport.sync(&self.image);
            self.mapped.reset();
            self.reset_input();
        }
    }

    fn redo(&mut self) {
        if self.image.redo() {
            self.passport.sync(&self.image);
            self.mapped.reset();
            self.reset_input();
        }
    }

    fn dirty(&self) -> bool {
        self.image.has_changes()
    }

    fn rom_changed(&self) -> bool {
        self.image.rom_mode() != self.image.saved_rom_mode()
    }

    fn change_count(&self) -> usize {
        self.image.differences().count()
            + self.changed_flags().count()
            + self.changed_mapped().len()
            + self.changed_passport().count()
            + usize::from(self.rom_changed())
    }

    fn changed_mapped(&self) -> Vec<(usize, usize, usize, f64, f64)> {
        if !self.dirty() {
            return Vec::new();
        }
        let mut changes = Vec::new();
        let mut seen = BTreeSet::new();
        for (parameter, cell) in self.image.differences() {
            seen.insert(parameter.spec().address + cell);
        }
        for (entry_index, entry) in self.catalog.entries().iter().enumerate() {
            if entry.kind == janvar_editor::j5::Kind::Bitfield {
                continue;
            }
            let Ok(calibration) = entry.numeric(&self.image) else {
                continue;
            };
            for cell in 0..calibration.count() {
                let address = calibration.byte_address(cell).expect("valid cell");
                let saved = calibration
                    .saved_value(&self.image, cell)
                    .expect("valid cell");
                let current = calibration.value(&self.image, cell).expect("valid cell");
                if saved != current && seen.insert(address) {
                    changes.push((entry_index, cell, address, saved, current));
                }
            }
        }
        changes
    }

    fn changed_flags(&self) -> impl Iterator<Item = (usize, &Flag)> {
        let mut seen = BTreeSet::new();
        self.catalog
            .entries()
            .iter()
            .enumerate()
            .flat_map(|(index, entry)| entry.flags.iter().map(move |flag| (index, flag)))
            .filter(move |(_, flag)| {
                self.image.flag(flag.address, flag.bit)
                    != self.image.saved_flag(flag.address, flag.bit)
                    && seen.insert((flag.address, flag.bit))
            })
    }

    fn changed_passport(&self) -> impl Iterator<Item = PassportField> {
        PassportField::ALL
            .into_iter()
            .filter(|&field| self.image.passport(field) != self.image.saved_passport(field))
    }

    fn input_events(&mut self, context: &egui::Context) {
        if context.input(|state| state.viewport().close_requested()) {
            if self.programming.running() {
                context.send_viewport_cmd(egui::ViewportCommand::CancelClose);
            } else if self.dirty() && !self.allow_close {
                context.send_viewport_cmd(egui::ViewportCommand::CancelClose);
                self.request(FileAction::Close);
            }
        }
        if self.confirm.is_some() || self.error.is_some() || self.programming.active() {
            return;
        }

        if let Some(path) = context.input(|state| {
            state
                .raw
                .dropped_files
                .iter()
                .find_map(|file| file.path.clone())
        }) {
            self.request(FileAction::Load(path));
        }
        if self.view.is_calibration() && starts_numeric_input(context) {
            self.start_table_input(context);
        }
        let editing = text_edit_focused(context);
        if !editing {
            if self.view == View::Data
                && !self.text_edit_was_focused
                && context.input(|input| input.key_pressed(Key::Escape))
            {
                self.cursor.range = None;
            }
            self.table_clipboard(context);
        }
        let keyboard_in_use = context.wants_keyboard_input();
        context.input_mut(|state| {
            if !menu::shortcut(self, state, editing)
                && !keyboard_in_use
                && self.view.is_calibration()
            {
                self.move_cell(state);
            }
        });
    }

    fn start_table_input(&mut self, context: &egui::Context) {
        if let Some(parameter) = self.parameter() {
            if parameter.writable() {
                self.input.clear();
                context
                    .memory_mut(|memory| memory.request_focus(egui::Id::new(inspector::INPUT_ID)));
            }
        } else if let Some(entry) = self.catalog.entries().get(self.selected)
            && entry.flags.is_empty()
            && let Ok(calibration) = entry.numeric(&self.image)
            && calibration.writable(&self.image)
        {
            self.mapped
                .start_input(context, entry.offset, self.cursor.index);
        }
    }

    fn move_cell(&mut self, input: &mut egui::InputState) {
        let Some(parameter) = self.parameter() else {
            return;
        };
        let columns = parameter.spec().shape.first().copied().unwrap_or(1);
        let index = self.cursor.index;
        let row = index / columns;
        let column = index % columns;
        let next = if input.consume_key(Modifiers::NONE, Key::ArrowRight) && column + 1 < columns {
            index + 1
        } else if input.consume_key(Modifiers::NONE, Key::ArrowLeft) && column > 0 {
            index - 1
        } else if input.consume_key(Modifiers::NONE, Key::ArrowDown)
            && index + columns < parameter.count()
        {
            index + columns
        } else if input.consume_key(Modifiers::NONE, Key::ArrowUp) && row > 0 {
            index - columns
        } else {
            index
        };
        if index != next {
            self.cursor
                .select(next, self.view == View::Data && input.modifiers.shift);
            self.reset_input();
        }
    }

    fn workbench(&mut self, ui: &mut egui::Ui) {
        chrome::tabs(self, ui);
        match self.view {
            View::Changes => return self.changes(ui),
            View::Rom => return self.rom_control(ui),
            View::Passport => {
                if let Err(error) = self.passport.show(ui, &mut self.image) {
                    self.error = Some(error);
                }
                return;
            }
            View::Data | View::Graph | View::Surface => {}
        }
        let Some(entry) = self.catalog.entries().get(self.selected) else {
            ui.label("Откройте карту J5 для этого BIN.");
            return;
        };
        ui.add_space(3.0);
        ui.label(theme::caption(labels::entry_path(entry).join("  /  ")));
        ui.label(
            RichText::new(labels::entry_title(entry))
                .size(24.0)
                .strong(),
        );
        ui.add_space(7.0);

        let selection = match self.parameter() {
            Some(parameter @ Parameter::Ignition) => {
                if self.view == View::Surface {
                    self.surface
                        .show(ui, &self.image, parameter, self.cursor.index)
                } else {
                    table::show(ui, &self.image, parameter, self.cursor)
                }
            }
            Some(Parameter::Idle) => {
                match curve::show(ui, &mut self.image, self.cursor, self.view == View::Graph) {
                    Ok(selection) => selection,
                    Err(error) => {
                        self.error = Some(error);
                        None
                    }
                }
            }
            Some(parameter @ (Parameter::FanOn | Parameter::FanOff)) => {
                let value = current(&self.image, parameter, 0);
                let cell = scalar::show(
                    ui,
                    value,
                    parameter.spec().unit,
                    self.cursor.range.is_some(),
                );
                if let Some(next) = scalar::fan_details(ui, &self.image, parameter) {
                    self.select_parameter(next, 0);
                }
                cell.clicked().then_some(Selection {
                    index: 0,
                    edit: cell.double_clicked(),
                })
            }
            None if !entry.flags.is_empty() => {
                if let Err(error) = flags::show(ui, &mut self.image, &entry.flags) {
                    self.error = Some(error);
                }
                None
            }
            _ => {
                let previous_cursor = self.cursor;
                let surface = (self.view == View::Surface).then_some(&mut self.surface);
                if let Err(error) = self.mapped.show(
                    ui,
                    &mut self.image,
                    entry,
                    &mut self.cursor,
                    self.view == View::Graph,
                    surface,
                ) {
                    self.error = Some(error);
                }
                if self.cursor != previous_cursor {
                    ui.ctx().request_repaint();
                }
                None
            }
        };
        if let Some(selection) = selection {
            let extend = self.view == View::Data && ui.input(|input| input.modifiers.shift);
            self.cursor.select(selection.index, extend);
            self.reset_input();
            ui.ctx().request_repaint();
            if selection.edit && !extend && self.parameter().is_some_and(Parameter::writable) {
                inspector::focus_value(ui, egui::Id::new(inspector::INPUT_ID), &self.input);
            }
        }
    }

    fn changes(&mut self, ui: &mut egui::Ui) {
        ui.label(RichText::new("Несохранённые правки").size(24.0).strong());
        if !self.dirty() {
            ui.label(theme::caption("Нет изменений"));
            return;
        }
        if self.rom_changed() {
            ui.horizontal(|ui| {
                if ui.link("Защита калибровок").clicked() {
                    self.view = View::Rom;
                }
                ui.label(format!(
                    "{} → {}",
                    labels::rom_title(self.image.saved_rom_mode()),
                    labels::rom_title(self.image.rom_mode())
                ));
            });
        }
        let mut selected = None;
        let mut flag_group = None;
        let mut mapped_cell = None;
        let mut open_passport = false;
        egui::ScrollArea::vertical().show(ui, |ui| {
            egui::Grid::new("changes table")
                .striped(true)
                .spacing([15.0, 5.0])
                .show(ui, |ui| {
                    for name in ["Калибровка", "Адрес", "Сохранено", "Текущее"]
                    {
                        ui.label(RichText::new(name).color(MUTED));
                    }
                    ui.end_row();
                    for field in self.changed_passport() {
                        open_passport |= ui.link(passport::title(field)).clicked();
                        let (address, _) = field.slot(self.image.profile());
                        ui.monospace(format!("0x{address:04X}"));
                        ui.monospace(self.image.saved_passport(field).unwrap_or_default());
                        ui.colored_label(ACCENT, self.image.passport(field).unwrap_or_default());
                        ui.end_row();
                    }
                    for (index, flag) in self.changed_flags() {
                        if ui.link(&flag.title).clicked() {
                            flag_group = Some(index);
                        }
                        ui.monospace(format!("0x{:04X}.{}", flag.address, flag.bit));
                        let label = |value| {
                            if value == Some(true) {
                                "Вкл"
                            } else {
                                "Выкл"
                            }
                        };
                        ui.label(label(self.image.saved_flag(flag.address, flag.bit)));
                        ui.colored_label(ACCENT, label(self.image.flag(flag.address, flag.bit)));
                        ui.end_row();
                    }
                    for (entry, cell, address, saved, current) in self.changed_mapped() {
                        if ui
                            .link(labels::entry_title(&self.catalog.entries()[entry]).as_ref())
                            .clicked()
                        {
                            mapped_cell = Some((entry, cell));
                        }
                        ui.monospace(format!("0x{address:04X}"));
                        ui.monospace(mapped::format_value(saved));
                        ui.colored_label(ACCENT, mapped::format_value(current));
                        ui.end_row();
                    }
                    for (parameter, cell) in self.image.differences() {
                        let spec = parameter.spec();
                        let info = reading(&self.image, parameter, cell);
                        if ui.link(labels::parameter_title(parameter)).clicked() {
                            selected = Some((parameter, cell));
                        }
                        ui.monospace(format!("0x{:04X}", info.address));
                        ui.monospace(labels::format_value(parameter, info.saved_value));
                        ui.colored_label(
                            ACCENT,
                            format!(
                                "{} {}",
                                labels::format_value(parameter, info.value),
                                spec.unit
                            ),
                        );
                        ui.end_row();
                    }
                });
        });
        if let Some((parameter, cell)) = selected {
            self.select_parameter(parameter, cell);
        } else if let Some((entry, cell)) = mapped_cell {
            self.select(entry);
            self.cursor.select(cell, false);
        } else if let Some(index) = flag_group {
            self.select(index);
        } else if open_passport {
            self.view = View::Passport;
        }
    }

    fn draw(&mut self, context: &egui::Context) {
        self.input_events(context);
        menu::show(self, context);
        chrome::footer(self, context);

        egui::SidePanel::left("calibrations")
            .default_width(230.0)
            .width_range(180.0..=330.0)
            .frame(theme::panel(PANEL, Margin::same(13)))
            .show(context, |ui| {
                let selected = if self.view == View::Passport {
                    Target::Passport
                } else {
                    Target::Calibration(self.selected)
                };
                match self.navigation.show(ui, &self.catalog, selected) {
                    Some(Target::Passport) => self.view = View::Passport,
                    Some(Target::Calibration(index)) => self.select(index),
                    None => {}
                }
            });

        if self.view.is_calibration() && !self.catalog.entries().is_empty() {
            egui::SidePanel::right("inspector")
                .default_width(264.0)
                .width_range(235.0..=350.0)
                .frame(theme::panel(PANEL, Margin::same(14)))
                .show(context, |ui| {
                    egui::ScrollArea::vertical()
                        .auto_shrink([false, false])
                        .show(ui, |ui| {
                            let Some(entry) = self.catalog.entries().get(self.selected) else {
                                return;
                            };
                            ui.heading("Свойства");
                            ui.separator();
                            if matches!(self.parameter(), None | Some(Parameter::AirCharge))
                                && entry.flags.is_empty()
                            {
                                if let Err(error) = self.mapped.show_value(
                                    ui,
                                    &mut self.image,
                                    entry,
                                    self.cursor.index,
                                ) {
                                    self.error = Some(error);
                                }
                                ui.separator();
                            }
                            if !self.image.can_edit_map()
                                || self.parameter() == Some(Parameter::AirCharge)
                            {
                                ui.monospace(entry.identifier());
                                return;
                            }
                            if let Some(action) = inspector::show(
                                ui,
                                &self.image,
                                entry,
                                self.cursor.index,
                                &mut self.input,
                                &mut self.batch_input,
                            ) {
                                self.apply(action);
                            }
                        });
                });
        }

        egui::CentralPanel::default()
            .frame(theme::panel(BG, Margin::same(15)))
            .show(context, |ui| self.workbench(ui));
        self.handle_actions(context);
        programming::show(self, context);
        self.text_edit_was_focused = text_edit_focused(context);
        #[cfg(target_os = "macos")]
        if let Some(menu) = &self.native_menu {
            menu.update(self);
        }
    }
}

impl eframe::App for Editor {
    fn update(&mut self, context: &egui::Context, _frame: &mut eframe::Frame) {
        self.draw(context);
    }
}

fn current(image: &Image, parameter: Parameter, index: usize) -> f64 {
    image
        .value(parameter, index)
        .expect("validated calibration index")
}

fn reading(image: &Image, parameter: Parameter, index: usize) -> Reading {
    image
        .reading(parameter, index)
        .expect("validated calibration index")
}

fn parse_value(input: &str) -> Result<f64, String> {
    input
        .trim()
        .replace(',', ".")
        .parse()
        .map_err(|_| "Введите число.".into())
}

fn starts_numeric_input(context: &egui::Context) -> bool {
    let typing_number = context.input(|input| {
        if input.modifiers.command || input.modifiers.ctrl {
            return false;
        }
        input.events.iter().any(|event| {
            matches!(event, egui::Event::Text(text)
                if !text.is_empty() && text.chars().all(|ch|
                    ch.is_ascii_digit() || matches!(ch, '+' | '-' | ',' | '.')))
        })
    });
    typing_number && !text_edit_focused(context)
}

fn text_edit_focused(context: &egui::Context) -> bool {
    context
        .memory(|memory| memory.focused())
        .is_some_and(|id| egui::TextEdit::load_state(context, id).is_some())
}

fn cancel_number_input(ui: &egui::Ui, id: egui::Id) -> bool {
    let was_editing = ui.memory(|memory| memory.has_focus(id) || memory.had_focus_last_frame(id));
    if was_editing && ui.input_mut(|input| input.consume_key(Modifiers::NONE, Key::Escape)) {
        ui.memory_mut(|memory| memory.surrender_focus(id));
        return true;
    }
    false
}

#[cfg(test)]
mod tests {
    use super::*;

    fn frame(
        editor: &mut Editor,
        ctx: &egui::Context,
        events: Vec<egui::Event>,
    ) -> egui::FullOutput {
        frame_with_modifiers(editor, ctx, events, Modifiers::NONE)
    }

    fn frame_with_modifiers(
        editor: &mut Editor,
        ctx: &egui::Context,
        events: Vec<egui::Event>,
        modifiers: Modifiers,
    ) -> egui::FullOutput {
        let input = egui::RawInput {
            screen_rect: Some(egui::Rect::from_min_size(
                egui::Pos2::ZERO,
                egui::vec2(1440.0, 900.0),
            )),
            events,
            modifiers,
            ..egui::RawInput::default()
        };
        let output = ctx.run(input, |ctx| editor.draw(ctx));
        assert!(!output.shapes.is_empty());
        output
    }

    fn key(key: Key) -> egui::Event {
        egui::Event::Key {
            key,
            physical_key: None,
            pressed: true,
            repeat: false,
            modifiers: Modifiers::NONE,
        }
    }

    fn pointer_button(pos: egui::Pos2, pressed: bool) -> egui::Event {
        egui::Event::PointerButton {
            pos,
            button: egui::PointerButton::Primary,
            pressed,
            modifiers: Modifiers::NONE,
        }
    }

    #[test]
    fn table_clipboard_copies_a_rectangle_and_pastes_from_its_top_left() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        let parameter = Parameter::Ignition;
        let indices = [0, 1, 2, 3, 16, 17, 18, 19];
        editor
            .image
            .set_cells(
                parameter,
                indices
                    .into_iter()
                    .zip([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]),
            )
            .unwrap();
        let values = |editor: &Editor| indices.map(|i| current(&editor.image, parameter, i));
        let before = values(&editor);

        frame(&mut editor, &ctx, vec![egui::Event::Paste("5 6".into())]);
        assert_eq!(values(&editor), [5.0, 6.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]);
        editor.undo();
        assert_eq!(values(&editor), before);
        let output = frame(&mut editor, &ctx, vec![key(Key::Escape)]);
        assert!(editor.cursor.range.is_none());

        let text_positions = |label: &str| {
            output
                .shapes
                .iter()
                .filter_map(|shape| match &shape.shape {
                    egui::Shape::Text(text) if text.galley.text() == label => {
                        Some(text.pos + text.galley.size() * 0.5)
                    }
                    _ => None,
                })
                .collect::<Vec<_>>()
        };
        let cell_at = |value: &str, row: usize| {
            let row_label = editor.image.load_axis(row).unwrap().to_string();
            let row_y = text_positions(&row_label)[0].y;
            text_positions(value)
                .into_iter()
                .find(|pos| (pos.y - row_y).abs() < 1.0)
                .unwrap()
        };
        let start = cell_at("2.0", 0);
        let end = cell_at("7.0", 1);
        for (point, modifiers) in [(start, Modifiers::NONE), (end, Modifiers::SHIFT)] {
            let button = |pressed| egui::Event::PointerButton {
                pos: point,
                button: egui::PointerButton::Primary,
                pressed,
                modifiers,
            };
            frame_with_modifiers(
                &mut editor,
                &ctx,
                vec![egui::Event::PointerMoved(point), button(true)],
                modifiers,
            );
            frame_with_modifiers(&mut editor, &ctx, vec![button(false)], modifiers);
        }
        let output = frame(&mut editor, &ctx, vec![egui::Event::Copy]);
        assert!(output.platform_output.commands.iter().any(|command| {
            matches!(command, egui::OutputCommand::CopyText(text) if text == "2\t3\n6\t7")
        }));

        frame(&mut editor, &ctx, vec![egui::Event::Paste("5 6".into())]);
        assert_eq!(values(&editor), [1.0, 5.0, 6.0, 4.0, 5.0, 6.0, 7.0, 8.0]);
        editor.undo();
        assert_eq!(values(&editor), before);

        editor.cursor.select(15, false);
        frame(&mut editor, &ctx, vec![egui::Event::Paste("5 6".into())]);
        assert!(editor.error.is_some());
        assert_eq!(values(&editor), before);
        editor.undo();
        assert!(!editor.dirty());
    }

    #[test]
    fn pasting_a_block_into_a_mapped_table_is_atomic() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        let entry = editor
            .catalog
            .entries()
            .iter()
            .position(|entry| entry.address == Some(0x83BA))
            .unwrap();
        editor.select(entry);
        let calibration = editor.catalog.entries()[entry]
            .numeric(&editor.image)
            .unwrap();
        let columns = calibration.shape()[0];
        let values = |image: &Image| {
            [0, 1, columns, columns + 1].map(|index| calibration.value(image, index).unwrap())
        };
        let before = values(&editor.image);
        frame(
            &mut editor,
            &ctx,
            vec![egui::Event::Paste("0.25\t0,75\n0.5\t0.125".into())],
        );
        assert_eq!(values(&editor.image), [0.25, 0.75, 0.5, 0.125]);
        editor.undo();
        assert_eq!(values(&editor.image), before);
        assert!(!editor.image.can_undo());
        frame(
            &mut editor,
            &ctx,
            vec![egui::Event::Paste("0.25 NaN".into())],
        );
        assert!(editor.error.is_some());
        assert_eq!(values(&editor.image), before);
        assert!(!editor.image.can_undo());
    }

    #[test]
    fn footer_shows_file_details_and_opens_checksums_on_click() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        let text_position = |output: &egui::FullOutput, label: &str| {
            output.shapes.iter().find_map(|shape| match &shape.shape {
                egui::Shape::Text(text) if text.galley.text() == label => {
                    Some(text.pos + text.galley.size() * 0.5)
                }
                _ => None,
            })
        };

        let output = frame(&mut editor, &ctx, Vec::new());
        let status = text_position(&output, "Суммы сходятся").unwrap();
        for label in [
            "BIN · J5v05l19.bin",
            "J5 · J5V05L19_738.j5",
            "CTP 3.21.0738",
        ] {
            let position = text_position(&output, label).unwrap();
            assert!(position.y > 850.0);
        }
        assert!(text_position(&output, "Контрольные суммы").is_none());

        frame(
            &mut editor,
            &ctx,
            vec![
                egui::Event::PointerMoved(status),
                pointer_button(status, true),
            ],
        );
        frame(&mut editor, &ctx, vec![pointer_button(status, false)]);
        let output = frame(&mut editor, &ctx, Vec::new());
        let title = text_position(&output, "Контрольные суммы").unwrap();
        assert!(title.y < status.y);
        assert!(text_position(&output, "Flash").is_some());
        assert!(text_position(&output, "В BIN").is_some());
        assert!(text_position(&output, "Расчёт").is_some());
    }

    #[test]
    fn typing_on_a_selected_table_cell_can_be_cancelled_or_applied() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        frame(&mut editor, &ctx, Vec::new());

        frame(&mut editor, &ctx, vec![egui::Event::Text("-25,3".into())]);
        assert_eq!(editor.input, "-25,3");
        assert!(!editor.dirty());
        frame(&mut editor, &ctx, vec![key(Key::Escape)]);
        assert_eq!(editor.input, "21.0");
        assert!(!editor.dirty());

        frame(&mut editor, &ctx, vec![egui::Event::Text("-25,3".into())]);
        frame(&mut editor, &ctx, vec![key(Key::Enter)]);
        assert_eq!(editor.input, "-25.5");
        assert_eq!(editor.change_count(), 1);
    }

    #[test]
    fn mapped_curve_supports_keyboard_entry_and_one_undo_per_drag() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        let index = editor
            .catalog
            .entries()
            .iter()
            .position(|entry| entry.id == [0x75, 0x92, 0x95, 0x6F])
            .unwrap();
        editor.select(index);
        let output = frame(&mut editor, &ctx, Vec::new());
        let apply_labels: Vec<_> = output
            .shapes
            .iter()
            .filter_map(|shape| match &shape.shape {
                egui::Shape::Text(text) if text.galley.text() == "Применить" => {
                    Some(text.pos.x)
                }
                _ => None,
            })
            .collect();
        assert_eq!(apply_labels.len(), 1);
        assert!(apply_labels[0] > 1100.0);
        assert!(!output.shapes.iter().any(|shape| matches!(
            &shape.shape,
            egui::Shape::Circle(circle) if circle.radius == 4.8
        )));
        let graph_tab = output
            .shapes
            .iter()
            .find_map(|shape| match &shape.shape {
                egui::Shape::Text(text) if text.galley.text() == "График 2D" => {
                    Some(text.pos + text.galley.size() * 0.5)
                }
                _ => None,
            })
            .unwrap();
        frame(
            &mut editor,
            &ctx,
            vec![
                egui::Event::PointerMoved(graph_tab),
                pointer_button(graph_tab, true),
            ],
        );
        let output = frame(&mut editor, &ctx, vec![pointer_button(graph_tab, false)]);
        assert!(editor.view == View::Graph);
        let selected_point = output.shapes.iter().find_map(|shape| match &shape.shape {
            egui::Shape::Circle(circle)
                if circle.fill == theme::SELECTED_CELL && circle.radius == 4.8 =>
            {
                Some(circle.center)
            }
            _ => None,
        });
        let point = selected_point.unwrap();
        let target = point - egui::vec2(0.0, 40.0);
        frame(
            &mut editor,
            &ctx,
            vec![
                egui::Event::PointerMoved(point),
                pointer_button(point, true),
            ],
        );
        frame(&mut editor, &ctx, vec![egui::Event::PointerMoved(target)]);
        assert!(!editor.dirty());
        frame(&mut editor, &ctx, vec![pointer_button(target, false)]);
        assert_eq!(editor.change_count(), 1);
        editor.undo();
        assert!(!editor.image.can_undo());
        assert!(!editor.dirty());

        frame(&mut editor, &ctx, vec![egui::Event::Text("100".into())]);
        frame(&mut editor, &ctx, vec![key(Key::Escape)]);
        assert!(!editor.dirty());
        frame(&mut editor, &ctx, vec![egui::Event::Text("100".into())]);
        frame(&mut editor, &ctx, vec![key(Key::Enter)]);
        let calibration = editor.catalog.entries()[index]
            .numeric(&editor.image)
            .unwrap();
        assert_eq!(
            calibration.value(&editor.image, 0),
            Some(35.0 * calibration.step())
        );
    }

    #[test]
    fn editing_a_surface_node_through_inspector_updates_the_grid() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        frame(&mut editor, &ctx, Vec::new());

        editor.view = View::Surface;
        editor.input.clear();
        ctx.memory_mut(|memory| memory.request_focus(egui::Id::new(inspector::INPUT_ID)));
        frame(&mut editor, &ctx, vec![egui::Event::Text("-25,3".into())]);
        assert_eq!(editor.input, "-25,3");
        frame(
            &mut editor,
            &ctx,
            vec![egui::Event::Key {
                key: Key::Enter,
                physical_key: None,
                pressed: true,
                repeat: false,
                modifiers: Modifiers::NONE,
            }],
        );
        assert_eq!(editor.image.bytes()[0x6AD9], 0xCD);
        assert_eq!(editor.change_count(), 1);

        editor.view = View::Data;
        frame(&mut editor, &ctx, Vec::new());
        assert_eq!(editor.input, "-25.5");
        assert_eq!(editor.cursor.index, 17);
        editor.undo();
        assert_eq!(editor.input, "21.0");
    }

    #[test]
    fn passport_and_checkbox_groups_render_and_appear_in_changes() {
        let ctx = egui::Context::default();
        let mut editor = Editor::new(&ctx, None);
        editor.view = View::Passport;
        frame(&mut editor, &ctx, Vec::new());
        editor
            .image
            .set_passport(PassportField::EngineType, "SAMARA")
            .unwrap();

        let configuration = editor
            .catalog
            .entries()
            .iter()
            .position(|entry| entry.id == [0x23, 0x8E, 0x74, 0x26])
            .unwrap();
        editor.select(configuration);
        frame(&mut editor, &ctx, Vec::new());
        editor.image.set_flag(0x6051, 1, false).unwrap();
        assert_eq!(editor.change_count(), 2);

        editor.view = View::Changes;
        frame(&mut editor, &ctx, Vec::new());
        editor.undo();
        assert_eq!(editor.change_count(), 1);
        editor.undo();
        assert!(!editor.dirty());
    }
}
