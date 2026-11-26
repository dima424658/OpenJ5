use super::{Editor, FileAction, theme};
use eframe::egui;
use janvar_editor::firmware::Image;
use janvar_flash::advanced::{FlashOptions, parse_checksum};
use janvar_flash::programmer::{
    self, DEFAULT_CONNECTION_TIMEOUT, Event, Operation, OperationError,
};
use janvar_flash::session::{Progress, Stage};
use std::fs;
use std::path::PathBuf;
use std::sync::mpsc::{self, Receiver, TryRecvError};
use std::time::Duration;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(super) enum Action {
    ReadFlash,
    WriteFlash,
    ClearFlash,
    ReadEeprom,
    WriteEeprom,
    ClearEeprom,
    PrepareFlash,
}

impl Action {
    pub(super) fn label(self) -> &'static str {
        match self {
            Self::ReadFlash => "Читать Flash…",
            Self::WriteFlash => "Записать Flash…",
            Self::ClearFlash => "Стереть Flash…",
            Self::ReadEeprom => "Читать EEPROM…",
            Self::WriteEeprom => "Записать EEPROM…",
            Self::ClearEeprom => "Очистить EEPROM…",
            Self::PrepareFlash => "Подготовить BIN…",
        }
    }

    fn confirm(self) -> &'static str {
        match self {
            Self::ReadFlash | Self::ReadEeprom => "Читать",
            Self::WriteFlash | Self::WriteEeprom => "Записать",
            Self::ClearFlash | Self::ClearEeprom => "Стереть",
            Self::PrepareFlash => "Сохранить",
        }
    }
}

pub(super) struct Programming {
    port: String,
    ports: Vec<String>,
    timeout: u32,
    port_error: Option<String>,
    dialog: Option<Dialog>,
}

impl Default for Programming {
    fn default() -> Self {
        Self {
            port: String::new(),
            ports: Vec::new(),
            timeout: DEFAULT_CONNECTION_TIMEOUT.as_secs() as u32,
            port_error: None,
            dialog: None,
        }
    }
}

enum Dialog {
    Setup(Setup),
    Running(Task),
    Finished(Result<Completion, String>),
}

impl Programming {
    pub(super) fn active(&self) -> bool {
        self.dialog.is_some()
    }

    pub(super) fn running(&self) -> bool {
        matches!(self.dialog, Some(Dialog::Running(_)))
    }

    pub(super) fn open(&mut self, action: Action) {
        if self.active() {
            return;
        }
        if action != Action::PrepareFlash {
            self.refresh_ports();
        }
        self.dialog = Some(Dialog::Setup(Setup::new(action)));
    }

    fn refresh_ports(&mut self) {
        match programmer::ports() {
            Ok(ports) => {
                if self.port.is_empty() && ports.len() == 1 {
                    self.port.clone_from(&ports[0]);
                }
                self.ports = ports;
                self.port_error = None;
            }
            Err(error) => self.port_error = Some(error.to_string()),
        }
    }

    fn connection(&mut self, ui: &mut egui::Ui) {
        ui.horizontal(|ui| {
            ui.label("Порт");
            ui.text_edit_singleline(&mut self.port);
            egui::ComboBox::from_id_salt("ecu-port-list")
                .selected_text("Порты")
                .show_ui(ui, |ui| {
                    for port in &self.ports {
                        ui.selectable_value(&mut self.port, port.clone(), port);
                    }
                });
            if ui.button("Обновить").clicked() {
                self.refresh_ports();
            }
        });
        if let Some(error) = &self.port_error {
            ui.label(error);
        }
        ui.horizontal(|ui| {
            ui.label("Ожидание ЭБУ");
            ui.add(
                egui::DragValue::new(&mut self.timeout)
                    .range(1..=u32::MAX)
                    .suffix(" с"),
            );
        });
    }
}

struct Setup {
    action: Action,
    current: bool,
    input: Option<PathBuf>,
    output: Option<PathBuf>,
    original: bool,
    remove_calibration_checks: bool,
    remove_error_mask: bool,
    checksum: String,
    starter_relay: Option<PathBuf>,
    error: Option<String>,
}

impl Setup {
    fn new(action: Action) -> Self {
        Self {
            action,
            current: true,
            input: None,
            output: None,
            original: false,
            remove_calibration_checks: false,
            remove_error_mask: false,
            checksum: String::new(),
            starter_relay: None,
            error: None,
        }
    }

    fn show(&mut self, ui: &mut egui::Ui, image: &Image, filename: &str) {
        match self.action {
            Action::WriteFlash | Action::PrepareFlash => {
                ui.horizontal(|ui| {
                    ui.selectable_value(&mut self.current, true, "Текущий BIN");
                    ui.selectable_value(&mut self.current, false, "Файл BIN");
                });
                if self.current {
                    ui.label(format!("{filename} · {}", image.software_id()));
                } else {
                    file_input(ui, &mut self.input, "Открыть BIN…", "BIN", &["bin", "bak"]);
                }
                if self.action == Action::PrepareFlash {
                    file_output(ui, &mut self.output, "prepared.bin", "BIN", "bin");
                }
                self.flash_options(ui);
            }
            Action::WriteEeprom => {
                file_input(
                    ui,
                    &mut self.input,
                    "Открыть EEPROM…",
                    "EEPROM",
                    &["eep", "bin"],
                );
            }
            Action::ReadFlash => file_output(ui, &mut self.output, "flash.bin", "BIN", "bin"),
            Action::ReadEeprom => file_output(ui, &mut self.output, "eeprom.eep", "EEPROM", "eep"),
            Action::ClearFlash => {
                ui.colored_label(theme::DANGER, "Вся Flash будет стёрта.");
            }
            Action::ClearEeprom => {
                ui.colored_label(theme::DANGER, "Вся EEPROM будет очищена.");
            }
        }
        let overwrite = match self.action {
            Action::WriteFlash => Some("Содержимое Flash будет заменено выбранным образом."),
            Action::WriteEeprom => Some("Содержимое EEPROM будет заменено выбранным дампом."),
            _ => None,
        };
        if let Some(text) = overwrite {
            ui.colored_label(theme::DANGER, text);
        }
        if let Some(error) = &self.error {
            ui.colored_label(theme::DANGER, error);
        }
    }

    fn flash_options(&mut self, ui: &mut egui::Ui) {
        let original = if self.action == Action::PrepareFlash {
            "Сохранить оригинал"
        } else {
            "Программировать оригинал"
        };
        ui.checkbox(&mut self.original, original);
        ui.add_enabled_ui(!self.original, |ui| {
            ui.checkbox(
                &mut self.remove_calibration_checks,
                "Удалить проверку калибровок",
            );
            ui.checkbox(
                &mut self.remove_error_mask,
                "Удалить дополнительную маску ошибок",
            );
            ui.horizontal(|ui| {
                ui.label("Контрольная сумма");
                ui.add(
                    egui::TextEdit::singleline(&mut self.checksum)
                        .hint_text("HEX")
                        .desired_width(90.0),
                );
            });
            file_input(
                ui,
                &mut self.starter_relay,
                "Выбрать шаблон реле стартера…",
                "BIN",
                &["bin"],
            );
            if self.starter_relay.is_some() && ui.button("Убрать шаблон").clicked() {
                self.starter_relay = None;
            }
        });
    }

    fn job(&self, image: &Image) -> Result<Job, String> {
        let output = || {
            let path = self.output.as_ref().ok_or("Выберите путь сохранения.")?;
            match path.try_exists() {
                Ok(false) => Ok(path.clone()),
                Ok(true) => Err(format!(
                    "Файл {} уже существует. Выберите новое имя.",
                    path.display()
                )),
                Err(error) => Err(format!("Не удалось проверить {}. {error}", path.display())),
            }
        };
        let input = || self.input.clone().ok_or("Выберите файл.".to_owned());
        let flash_input = || {
            if self.current {
                Ok(Source::Current(image.to_bin()))
            } else {
                input().map(Source::File)
            }
        };
        let options = || {
            let checksum = self.checksum.trim();
            Ok::<_, String>(FlashOptions {
                original: self.original,
                remove_calibration_checks: self.remove_calibration_checks,
                remove_error_mask: self.remove_error_mask,
                checksum: if self.original || checksum.is_empty() {
                    None
                } else {
                    Some(parse_checksum(checksum).map_err(|_| "Введите контрольную сумму в HEX.")?)
                },
                starter_relay: self.starter_relay.clone(),
            })
        };
        Ok(match self.action {
            Action::ReadFlash => Job::Serial(Operation::ReadFlash(output()?)),
            Action::ReadEeprom => Job::Serial(Operation::ReadEeprom(output()?)),
            Action::ClearFlash => Job::Serial(Operation::ClearFlash),
            Action::ClearEeprom => Job::Serial(Operation::ClearEeprom),
            Action::WriteFlash => Job::WriteFlash(flash_input()?, options()?),
            Action::WriteEeprom => Job::WriteEeprom(input()?),
            Action::PrepareFlash => Job::Prepare(flash_input()?, output()?, options()?),
        })
    }
}

fn file_input(
    ui: &mut egui::Ui,
    selected: &mut Option<PathBuf>,
    label: &str,
    filter: &str,
    extensions: &[&str],
) {
    ui.horizontal(|ui| {
        if ui.button(label).clicked()
            && let Some(path) = rfd::FileDialog::new()
                .add_filter(filter, extensions)
                .pick_file()
        {
            *selected = Some(path);
        }
        if let Some(path) = selected {
            ui.label(
                path.file_name()
                    .unwrap_or(path.as_os_str())
                    .to_string_lossy(),
            )
            .on_hover_text(path.display().to_string());
        }
    });
}

fn file_output(
    ui: &mut egui::Ui,
    selected: &mut Option<PathBuf>,
    name: &str,
    filter: &str,
    extension: &str,
) {
    ui.horizontal(|ui| {
        if ui.button("Сохранить в…").clicked()
            && let Some(mut path) = rfd::FileDialog::new()
                .add_filter(filter, &[extension])
                .set_file_name(name)
                .save_file()
        {
            if path.extension().is_none() {
                path.set_extension(extension);
            }
            *selected = Some(path);
        }
        if let Some(path) = selected {
            ui.label(
                path.file_name()
                    .unwrap_or(path.as_os_str())
                    .to_string_lossy(),
            )
            .on_hover_text(path.display().to_string());
        }
    });
}

enum Source {
    Current(Vec<u8>),
    File(PathBuf),
}

impl Source {
    fn bytes(self) -> std::io::Result<Vec<u8>> {
        match self {
            Self::Current(bytes) => Ok(bytes),
            Self::File(path) => fs::read(path),
        }
    }
}

enum Job {
    Serial(Operation),
    WriteFlash(Source, FlashOptions),
    WriteEeprom(PathBuf),
    Prepare(Source, PathBuf, FlashOptions),
}

struct Completion {
    message: String,
    open_bin: Option<PathBuf>,
}

impl Job {
    fn run(
        self,
        port: &str,
        timeout: Duration,
        report: impl FnMut(Event) + 'static,
    ) -> Result<Completion, OperationError> {
        let operation = match self {
            Self::Serial(operation) => operation,
            Self::WriteFlash(source, options) => {
                Operation::WriteFlash(options.prepare(source.bytes()?)?)
            }
            Self::WriteEeprom(path) => {
                Operation::WriteEeprom(programmer::eeprom_image(fs::read(path)?)?)
            }
            Self::Prepare(source, path, options) => {
                let checksum = options.save_prepared(source.bytes()?, &path)?;
                let mut message = format!("Сохранён {}", path.display());
                if let Some(checksum) = checksum {
                    message.push_str(&format!(" · контрольная сумма {checksum:04X}"));
                }
                return Ok(Completion {
                    message,
                    open_bin: Some(path),
                });
            }
        };
        let size = programmer::run(&operation, port, timeout, report)?;
        let (message, open_bin) = match operation {
            Operation::ReadFlash(path) => (
                format!("Сохранено {size} байт Flash в {}", path.display()),
                Some(path),
            ),
            Operation::ReadEeprom(path) => (
                format!("Сохранено {size} байт EEPROM в {}", path.display()),
                None,
            ),
            Operation::WriteFlash(_) => (
                format!("Запись Flash подтверждена чтением {size} байт"),
                None,
            ),
            Operation::WriteEeprom(_) => (
                format!("Запись EEPROM подтверждена чтением {size} байт"),
                None,
            ),
            Operation::ClearFlash => (
                format!("Стирание Flash подтверждено чтением {size} байт"),
                None,
            ),
            Operation::ClearEeprom => (
                format!("Очистка EEPROM подтверждена чтением {size} байт"),
                None,
            ),
        };
        Ok(Completion { message, open_bin })
    }
}

enum Update {
    Event(Event),
    Done(Result<Completion, String>),
}

struct Task {
    action: Action,
    port: String,
    receiver: Receiver<Update>,
    status: &'static str,
    progress: Option<Progress>,
    readback: bool,
    flags: Option<u8>,
}

impl Task {
    fn start(
        action: Action,
        job: Job,
        port: String,
        timeout: Duration,
        context: &egui::Context,
    ) -> std::io::Result<Self> {
        let (sender, receiver) = mpsc::channel();
        let context = context.clone();
        let active_port = port.clone();
        std::thread::Builder::new()
            .name("ecu-programming".into())
            .spawn(move || {
                let events = sender.clone();
                let repaint = context.clone();
                let result = job
                    .run(&active_port, timeout, move |event| {
                        if events.send(Update::Event(event)).is_ok() {
                            repaint.request_repaint();
                        }
                    })
                    .map_err(|error| error.to_string());
                if sender.send(Update::Done(result)).is_ok() {
                    context.request_repaint();
                }
            })?;
        Ok(Self {
            action,
            port,
            receiver,
            status: if action == Action::PrepareFlash {
                "Подготовка BIN"
            } else {
                "Открытие порта"
            },
            progress: None,
            readback: false,
            flags: None,
        })
    }

    fn poll(&mut self) -> Option<Result<Completion, String>> {
        loop {
            match self.receiver.try_recv() {
                Ok(Update::Done(result)) => return Some(result),
                Ok(Update::Event(Event::Waiting)) if !self.readback => {
                    self.status = "Ожидание ЭБУ в режиме программирования"
                }
                Ok(Update::Event(Event::Waiting)) => {}
                Ok(Update::Event(Event::Connected(flags))) => self.flags = Some(flags),
                Ok(Update::Event(Event::Progress(progress))) => {
                    self.status = stage_label(progress.stage);
                    self.progress = Some(progress);
                }
                Ok(Update::Event(Event::AwaitingPowerCycle(_))) => {
                    self.readback = true;
                    self.progress = None;
                    self.status = "Перезапустите питание ЭБУ в режиме программирования для контрольного чтения.";
                }
                Err(TryRecvError::Empty) => return None,
                Err(TryRecvError::Disconnected) => return Some(Err("Операция прервана.".into())),
            }
        }
    }

    fn show(&self, ui: &mut egui::Ui) {
        ui.heading(self.action.label());
        if self.action != Action::PrepareFlash {
            ui.label(&self.port);
        }
        if let Some(flags) = self.flags {
            ui.label(format!("Флаги ЭБУ {flags:02X}"));
        }
        ui.label(self.status);
        if let Some(progress) = self.progress {
            ui.add(
                egui::ProgressBar::new(progress.completed as f32 / progress.total as f32)
                    .text(format!("{} / {}", progress.completed, progress.total)),
            );
        }
    }
}

fn stage_label(stage: Stage) -> &'static str {
    match stage {
        Stage::Bootstrap => "Загрузка бутстрапа",
        Stage::ReadFlash => "Чтение Flash",
        Stage::EraseFlash => "Стирание Flash",
        Stage::WriteFlash => "Запись Flash",
        Stage::FinishFlash => "Завершение записи Flash",
        Stage::ReadEeprom => "Чтение EEPROM",
        Stage::WriteEeprom => "Запись EEPROM",
        Stage::ClearEeprom => "Очистка EEPROM",
    }
}

pub(super) fn show(editor: &mut Editor, context: &egui::Context) {
    let Some(dialog) = editor.programming.dialog.take() else {
        return;
    };
    editor.programming.dialog = match dialog {
        Dialog::Setup(mut setup) => {
            let filename = editor.file_name();
            let modal = egui::Modal::new(egui::Id::new("ecu-setup")).show(context, |ui| {
                ui.set_width(420.0);
                ui.heading(setup.action.label());
                if setup.action != Action::PrepareFlash {
                    editor.programming.connection(ui);
                }
                setup.show(ui, &editor.image, &filename);
                ui.horizontal(|ui| {
                    let cancel = ui.button("Отмена").clicked();
                    let start = ui.add(theme::primary(setup.action.confirm())).clicked();
                    (cancel, start)
                })
                .inner
            });
            let (cancel, start) = modal.inner;
            if start {
                let port = editor.programming.port.trim().to_owned();
                let timeout = Duration::from_secs(u64::from(editor.programming.timeout));
                let launched = setup.job(&editor.image).and_then(|job| {
                    if setup.action != Action::PrepareFlash && port.is_empty() {
                        return Err("Укажите последовательный порт.".into());
                    }
                    Task::start(setup.action, job, port, timeout, context)
                        .map_err(|error| error.to_string())
                });
                match launched {
                    Ok(task) => Some(Dialog::Running(task)),
                    Err(error) => {
                        setup.error = Some(error);
                        Some(Dialog::Setup(setup))
                    }
                }
            } else if cancel || modal.should_close() {
                None
            } else {
                Some(Dialog::Setup(setup))
            }
        }
        Dialog::Running(mut task) => {
            if let Some(result) = task.poll() {
                context.request_repaint();
                Some(Dialog::Finished(result))
            } else {
                egui::Modal::new(egui::Id::new("ecu-progress")).show(context, |ui| {
                    ui.set_width(420.0);
                    task.show(ui);
                });
                Some(Dialog::Running(task))
            }
        }
        Dialog::Finished(result) => {
            let modal = egui::Modal::new(egui::Id::new("ecu-result")).show(context, |ui| {
                ui.set_max_width(420.0);
                let open = match &result {
                    Ok(completion) => {
                        ui.label(&completion.message);
                        if let Some(path) = &completion.open_bin {
                            ui.button("Открыть BIN в редакторе")
                                .clicked()
                                .then(|| path.clone())
                        } else {
                            None
                        }
                    }
                    Err(error) => {
                        ui.colored_label(theme::DANGER, error);
                        None
                    }
                };
                (ui.button("Закрыть").clicked(), open)
            });
            let close = modal.inner.0 || modal.should_close();
            let open = modal.inner.1;
            if let Some(path) = open {
                editor.request(FileAction::Load(path));
                return;
            }
            if close {
                None
            } else {
                Some(Dialog::Finished(result))
            }
        }
    };
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn read_detects_an_existing_destination_before_opening_a_port() {
        let directory = tempfile::tempdir().unwrap();
        let output = directory.path().join("flash.bin");
        fs::write(&output, b"saved backup").unwrap();
        let image = Image::new(crate::demo::SAMPLE.to_vec()).unwrap();
        let mut setup = Setup::new(Action::ReadFlash);
        setup.output = Some(output);
        assert!(matches!(setup.job(&image), Err(error) if error.contains("уже существует")));
    }

    #[test]
    fn prepare_saves_the_current_bin_without_touching_a_port() {
        let directory = tempfile::tempdir().unwrap();
        let output = directory.path().join("prepared.bin");
        let image = Image::new(crate::demo::SAMPLE.to_vec()).unwrap();
        let mut setup = Setup::new(Action::PrepareFlash);
        setup.output = Some(output.clone());
        setup.checksum = "0x4d22".into();
        let job = setup.job(&image).unwrap();
        let completed = job
            .run("", Duration::ZERO, |_| panic!("serial event"))
            .unwrap();

        assert_eq!(completed.open_bin, Some(output.clone()));
        let bytes = fs::read(&output).unwrap();
        assert_eq!(&bytes[bytes.len() - 2..], &0x4d22_u16.to_le_bytes());
        assert!(
            Image::new(bytes)
                .unwrap()
                .checksums()
                .iter()
                .all(|sum| sum.valid())
        );
    }
}
