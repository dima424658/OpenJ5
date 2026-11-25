use super::labels;
use super::table;
use super::theme::{self, ACCENT, SELECTED_CELL};
use eframe::egui::{self, Ui};
use janvar_editor::firmware::{Image, Parameter};

pub fn show(ui: &mut Ui, value: f64, unit: &str, selected: bool) -> egui::Response {
    ui.label(theme::caption(if unit.is_empty() {
        "Значение"
    } else {
        unit
    }));
    let fill = if selected {
        SELECTED_CELL
    } else {
        theme::heat(0.5)
    };
    table::number_cell(ui, value, fill, 112.0)
}

pub fn fan_details(ui: &mut Ui, image: &Image, parameter: Parameter) -> Option<Parameter> {
    let spec = parameter.spec();
    ui.add_space(6.0);
    ui.label(theme::caption(format!(
        "Температура охлаждающей жидкости · диапазон {} … {} {} · шаг {}",
        spec.min, spec.max, spec.unit, spec.step
    )));
    let other = if parameter == Parameter::FanOn {
        Parameter::FanOff
    } else {
        Parameter::FanOn
    };
    let label = format!(
        "{}    {} °C",
        labels::parameter_title(other),
        labels::format_value(other, super::current(image, other, 0))
    );
    if super::current(image, Parameter::FanOn, 0) <= super::current(image, Parameter::FanOff, 0) {
        ui.colored_label(
            ACCENT,
            "Порог включения ниже порога выключения или равен ему.",
        );
    }
    ui.button(label).clicked().then_some(other)
}
