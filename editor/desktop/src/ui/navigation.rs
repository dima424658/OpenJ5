use super::labels;
use super::theme::{self, ACCENT, MUTED};
use eframe::egui::{self, RichText, Ui};
use janvar_editor::j5::Catalog;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Target {
    Passport,
    Calibration(usize),
}

#[derive(Default)]
struct Folder {
    name: String,
    folders: Vec<Self>,
    entries: Vec<usize>,
}

impl Folder {
    fn insert(&mut self, path: &[&str], index: usize) {
        if let Some((name, subpath)) = path.split_first() {
            let child = match self.folders.iter().position(|folder| folder.name == *name) {
                Some(position) => &mut self.folders[position],
                None => {
                    self.folders.push(Self {
                        name: (*name).to_owned(),
                        ..Self::default()
                    });
                    self.folders.last_mut().expect("inserted folder")
                }
            };
            child.insert(subpath, index);
        } else {
            self.entries.push(index);
        }
    }

    fn show(&self, ui: &mut Ui, catalog: &Catalog, selected: Target, next: &mut Option<Target>) {
        for folder in &self.folders {
            egui::CollapsingHeader::new(&folder.name)
                .id_salt(&folder.name)
                .show(ui, |ui| folder.show(ui, catalog, selected, next));
        }
        for &index in &self.entries {
            entry_button(ui, catalog, selected, index, next);
        }
    }
}

pub struct Navigation {
    tree: Folder,
    filter: String,
}

impl Navigation {
    pub fn new(catalog: &Catalog) -> Self {
        let mut tree = Folder::default();
        for (index, entry) in catalog.entries().iter().enumerate() {
            tree.insert(&labels::entry_path(entry), index);
        }
        Self {
            tree,
            filter: String::new(),
        }
    }

    pub fn show(&mut self, ui: &mut Ui, catalog: &Catalog, selected: Target) -> Option<Target> {
        ui.heading("Калибровки");
        ui.add_space(3.0);
        ui.add(
            egui::TextEdit::singleline(&mut self.filter)
                .hint_text("Имя, адрес или ID")
                .desired_width(f32::INFINITY),
        );
        ui.separator();

        let mut next = None;
        let query = self.filter.trim().to_lowercase();
        egui::ScrollArea::vertical()
            .id_salt("calibration navigation")
            .auto_shrink([false, false])
            .show(ui, |ui| {
                let passport_matches = "паспорт прошивки".contains(&query);
                if passport_matches
                    && row(ui, selected == Target::Passport, "Паспорт прошивки").clicked()
                {
                    next = Some(Target::Passport);
                }
                if !query.is_empty() {
                    let mut found = passport_matches;
                    for (index, entry) in catalog.entries().iter().enumerate() {
                        if labels::entry_title(entry).to_lowercase().contains(&query)
                            || entry
                                .path
                                .iter()
                                .any(|name| name.to_lowercase().contains(&query))
                            || entry.identifier().to_lowercase().contains(&query)
                            || entry
                                .address
                                .is_some_and(|address| format!("0x{address:04x}").contains(&query))
                        {
                            found = true;
                            entry_button(ui, catalog, selected, index, &mut next);
                        }
                    }
                    if !found {
                        ui.label(theme::caption("Ничего не найдено"));
                    }
                } else {
                    self.tree.show(ui, catalog, selected, &mut next);
                }
            });
        next
    }
}

fn row(ui: &mut Ui, selected: bool, title: &str) -> egui::Response {
    let mut text = RichText::new(title);
    if selected {
        text = text.color(ACCENT).strong();
    }
    ui.add_sized(
        [ui.available_width(), 32.0],
        egui::Button::selectable(selected, (text, egui::Atom::grow())).wrap(),
    )
}

fn entry_button(
    ui: &mut Ui,
    catalog: &Catalog,
    selected: Target,
    index: usize,
    next: &mut Option<Target>,
) {
    let entry = &catalog.entries()[index];
    let target = Target::Calibration(index);
    if row(ui, selected == target, labels::entry_title(entry).as_ref())
        .on_hover_ui(|ui| {
            ui.label(labels::entry_title(entry).as_ref());
            ui.label(RichText::new(labels::entry_path(entry).join(" / ")).color(MUTED));
        })
        .clicked()
    {
        *next = Some(target);
    }
}
