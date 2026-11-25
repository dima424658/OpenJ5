use janvar_editor::firmware::{ChecksumKind, Parameter, RomMode};
use janvar_editor::j5::{Entry, Kind};
use std::borrow::Cow;

pub fn parameter_title(parameter: Parameter) -> &'static str {
    match parameter {
        Parameter::Ignition => "УОЗ для экономичного режима",
        Parameter::Idle => "Желаемые обороты ХХ",
        Parameter::FanOn => "Температура включения",
        Parameter::FanOff => "Температура выключения",
        Parameter::AirCharge => "Базовое цикловое наполнение",
    }
}

pub fn format_value(parameter: Parameter, value: f64) -> String {
    if parameter == Parameter::Ignition {
        format!("{value:.1}")
    } else {
        format!("{value:.0}")
    }
}

pub fn format_table_value(value: f64) -> String {
    let rounded = format!("{value:.2}");
    trim_decimal(&rounded).to_owned()
}

pub(super) fn trim_decimal(value: &str) -> &str {
    let label = value.trim_end_matches('0').trim_end_matches('.');
    if label == "-0" { "0" } else { label }
}

pub fn parameter_description(parameter: Parameter) -> &'static str {
    match parameter {
        Parameter::Ignition => {
            "Январь 5.1 интерполирует сырые байты этой таблицы без знака. При переходе через 0° промежуточный результат может отличаться от линейного среднего."
        }
        Parameter::Idle => {
            "Узлы от −40 до 150 °C с шагом 5 °C. В разобранной ветви ЭБУ использует узлы до 145 °C включительно."
        }
        Parameter::FanOn | Parameter::FanOff => {
            "Один байт. Температура в градусах Цельсия равна значению байта минус 40."
        }
        Parameter::AirCharge => {
            "Сырые значения. Масштаб наполнения и разметка оси дросселя ещё проверяются."
        }
    }
}

pub fn entry_title(entry: &Entry) -> Cow<'_, str> {
    entry.title.as_deref().map_or_else(
        || Cow::Owned(format!("{} {}", kind_title(entry.kind), entry.identifier())),
        Cow::Borrowed,
    )
}

pub fn entry_path(entry: &Entry) -> Vec<&str> {
    if !entry.path.is_empty() {
        entry.path.iter().map(String::as_str).collect()
    } else if entry.title.is_some() {
        vec!["Общие"]
    } else {
        vec!["Несопоставленные записи"]
    }
}

pub fn kind_title(kind: Kind) -> &'static str {
    match kind {
        Kind::Folder => "Раздел",
        Kind::Curve => "Кривая",
        Kind::Table => "Таблица",
        Kind::Scalar => "Число",
        Kind::Bitfield => "Битовая маска",
    }
}

pub fn checksum_title(kind: ChecksumKind) -> &'static str {
    match kind {
        ChecksumKind::Flash => "Flash",
        ChecksumKind::FlashComplement => "Дополнение Flash",
        ChecksumKind::Calibration6000 => "Калибровки 6000–7FFF",
        ChecksumKind::Calibration8000 => "Калибровки 8000–9FFF",
    }
}

pub fn rom_title(mode: RomMode) -> &'static str {
    match mode {
        RomMode::Stock => "Штатный",
        RomMode::CtpDisabled => "Отключён через CTP",
        RomMode::VendorDisabled => "Вариант L19a",
        RomMode::Unknown => "Неизвестная модификация",
        RomMode::Unsupported => "Доступно для L19",
    }
}
