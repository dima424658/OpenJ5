use super::theme::{self, DANGER};
use super::{Editor, FileAction};
use crate::demo;
use eframe::egui;
use janvar_editor::firmware::{self, SaveMode};
use janvar_editor::j5::Catalog;
use std::path::{Path, PathBuf};

impl Editor {
    fn current_path(&self) -> Option<&Path> {
        self.destination.as_deref().or(self.source.as_deref())
    }

    pub(super) fn file_name(&self) -> String {
        self.current_path().and_then(Path::file_name).map_or_else(
            || "J5v05l19.bin".into(),
            |name| name.to_string_lossy().into_owned(),
        )
    }

    pub(super) fn request(&mut self, action: FileAction) {
        self.pending = Some(action);
    }

    pub(super) fn open_bin(&mut self, path: PathBuf) -> Result<(), String> {
        let image = firmware::open(&path).map_err(|error| error.to_string())?;
        self.image = image;
        self.passport.sync(&self.image);
        self.source = Some(path.clone());
        self.destination = None;
        self.reference_0738 = None;
        let adjacent = path
            .parent()
            .and_then(|dir| std::fs::read_dir(dir).ok())
            .and_then(|files| {
                files
                    .filter_map(Result::ok)
                    .map(|file| file.path())
                    .filter(|map| is_map(map) && matches_software(map, self.image.software_id()))
                    .filter(|map| {
                        std::fs::read(map).is_ok_and(|data| data.starts_with(b"CTM\x03\x01\x00"))
                    })
                    .min_by_key(|map| {
                        let exact =
                            map.file_stem()
                                .and_then(|stem| stem.to_str())
                                .is_some_and(|stem| {
                                    stem.eq_ignore_ascii_case(self.image.software_id())
                                });
                        (!exact, map.clone())
                    })
            });
        if let Some((catalog, name)) = demo::reference(self.image.profile()) {
            self.set_catalog(catalog, name.into());
        } else {
            self.set_catalog(Catalog::empty(), "Карта не загружена".into());
            if let Some(map) = adjacent {
                self.open_map(&map)?;
            }
        }
        self.notice = Some(format!("Открыт {}", self.file_name()));
        Ok(())
    }

    fn open_map(&mut self, path: &Path) -> Result<(), String> {
        let bytes =
            std::fs::read(path).map_err(|error| format!("Не удалось прочитать карту. {error}"))?;
        let mut catalog = Catalog::parse(&bytes).map_err(|error| error.to_string())?;
        if let Some((reference, _)) = demo::reference(self.image.profile()) {
            catalog = catalog
                .with_reference(&reference)
                .map_err(|error| error.to_string())?;
        } else {
            if !matches_software(path, self.image.software_id()) {
                return Err(format!(
                    "Имя карты должно соответствовать идентификатору ПО {}.",
                    self.image.software_id()
                ));
            }
            if catalog.build() == janvar_editor::j5::Build::Ctp0923 {
                let reference = self
                    .reference_0738
                    .as_ref()
                    .ok_or_else(|| "Сначала откройте карту 0738 для этого BIN.".to_string())?;
                catalog = catalog
                    .with_reference(reference)
                    .map_err(|error| error.to_string())?;
            } else {
                self.reference_0738 =
                    Some(Catalog::parse(&bytes).map_err(|error| error.to_string())?);
            }
        }
        let name = path
            .file_name()
            .unwrap_or(path.as_os_str())
            .to_string_lossy()
            .into_owned();
        self.notice = Some(format!("Загружена карта {name}"));
        self.set_catalog(catalog, name);
        Ok(())
    }

    fn save(&mut self, as_copy: bool) -> Result<(), String> {
        if !as_copy && let Some(path) = self.current_path() {
            return self.write_bin(path.to_owned(), SaveMode::Replace);
        }
        let name = self.file_name();
        let base = Path::new(&name)
            .file_stem()
            .unwrap_or_default()
            .to_string_lossy();
        let mut dialog = rfd::FileDialog::new()
            .add_filter("BIN", &["bin"])
            .set_file_name(format!("{base}-edited.bin"));
        if let Some(parent) = self
            .current_path()
            .and_then(Path::parent)
            .filter(|parent| !parent.as_os_str().is_empty())
        {
            dialog = dialog.set_directory(parent);
        }
        let Some(path) = dialog.save_file() else {
            return Ok(());
        };
        let path = with_extension(path);
        let target = path.canonicalize().ok();
        if target.is_some()
            && self
                .source
                .as_ref()
                .and_then(|source| source.canonicalize().ok())
                == target
        {
            return Err("Выберите другой путь для команды «Сохранить как…».".into());
        }
        let replace = self.destination.as_ref().is_some_and(|destination| {
            destination == &path || (target.is_some() && destination.canonicalize().ok() == target)
        });
        let mode = if replace {
            SaveMode::Replace
        } else {
            SaveMode::Create
        };
        self.write_bin(path, mode)
    }

    fn write_bin(&mut self, path: PathBuf, mode: SaveMode) -> Result<(), String> {
        let error_message = |error| format!("Не удалось сохранить {}. {error}", path.display());
        let resolved;
        let target = if mode == SaveMode::Replace && path.is_symlink() {
            resolved = path.canonicalize().map_err(error_message)?;
            &resolved
        } else {
            &path
        };
        self.image.save(target, mode).map_err(error_message)?;
        self.destination = Some(path);
        self.notice = Some(format!("Сохранён {}", self.file_name()));
        Ok(())
    }

    fn perform(&mut self, action: FileAction, context: &egui::Context) -> Result<(), String> {
        match action {
            FileAction::OpenBin => {
                if let Some(path) = rfd::FileDialog::new()
                    .add_filter("BIN", &["bin", "bak"])
                    .pick_file()
                {
                    self.open_bin(path)?;
                }
            }
            FileAction::OpenMap => {
                if let Some(path) = rfd::FileDialog::new()
                    .add_filter("Калибровки J5", &["j5"])
                    .pick_file()
                {
                    self.open_map(&path)?;
                }
            }
            FileAction::Load(path) if is_map(&path) => self.open_map(&path)?,
            FileAction::Load(path) => self.open_bin(path)?,
            FileAction::Save => self.save(false)?,
            FileAction::SaveAs => self.save(true)?,
            FileAction::Close => {
                self.allow_close = true;
                context.send_viewport_cmd(egui::ViewportCommand::Close);
            }
        }
        Ok(())
    }

    pub(super) fn handle_actions(&mut self, context: &egui::Context) {
        if let Some(action) = self.pending.take() {
            let replaces = match &action {
                FileAction::OpenBin | FileAction::Close => true,
                FileAction::Load(path) => !is_map(path),
                _ => false,
            };
            if replaces && self.dirty() {
                self.confirm = Some(action);
            } else {
                self.error = self.perform(action, context).err();
            }
        }
        if self.confirm.is_some() {
            let answer =
                egui::Modal::new(egui::Id::new("discard unsaved changes")).show(context, |ui| {
                    ui.set_max_width(380.0);
                    ui.heading("Несохранённые изменения");
                    ui.label("При продолжении правки текущего BIN будут потеряны.");
                    ui.horizontal(|ui| {
                        if ui.button("Отмена").clicked() {
                            Some(false)
                        } else if ui
                            .add(theme::primary("Продолжить без сохранения"))
                            .clicked()
                        {
                            Some(true)
                        } else {
                            None
                        }
                    })
                    .inner
                });
            if let Some(accepted) = answer.inner {
                let action = self.confirm.take().expect("pending confirmation");
                if accepted {
                    self.error = self.perform(action, context).err();
                }
            } else if answer.should_close() {
                self.confirm = None;
            }
        }
        if let Some(error) = &self.error {
            let dialog = egui::Modal::new(egui::Id::new("editor error")).show(context, |ui| {
                ui.set_max_width(400.0);
                ui.colored_label(DANGER, error);
                ui.button("Закрыть").clicked()
            });
            if dialog.inner || dialog.should_close() {
                self.error = None;
            }
        }
    }
}

fn with_extension(mut path: PathBuf) -> PathBuf {
    if path.extension().is_none() {
        path.set_extension("bin");
    }
    path
}

fn is_map(path: &Path) -> bool {
    path.extension()
        .is_some_and(|ext| ext.eq_ignore_ascii_case("j5"))
}

fn matches_software(path: &Path, software: &str) -> bool {
    let Some(stem) = path.file_stem().and_then(|stem| stem.to_str()) else {
        return false;
    };
    let prefix = stem.split(['_', ' ']).next().unwrap_or_default();
    prefix.eq_ignore_ascii_case(software)
}
