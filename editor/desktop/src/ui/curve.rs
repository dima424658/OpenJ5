use super::Selection;
use super::cells::{CellRange, Cursor};
use super::curve_plot::drag_point;
use super::labels;
use super::theme::{self, BG, LINE, MINT, MUTED, SELECTED_CELL};
use eframe::egui::{self, Align2, FontId, Pos2, Sense, Shape, Stroke, Ui, pos2, vec2};
use janvar_editor::firmware::{Image, Parameter, idle_temperature};

pub fn show(
    ui: &mut Ui,
    image: &mut Image,
    cursor: Cursor,
    show_graph: bool,
) -> Result<Option<Selection>, String> {
    let mut values: Vec<_> = (0..Parameter::Idle.count())
        .map(|index| super::current(image, Parameter::Idle, index))
        .collect();
    let mut selection = if show_graph {
        show_plot(ui, image, cursor.index, &mut values)?
    } else {
        ui.label(theme::caption("Температура, °C"));
        None
    };
    let coordinates = (0..values.len())
        .map(|index| format!("{} °C", idle_temperature(index).expect("idle node")));
    let active = selection.map_or(cursor.index, |selection| selection.index);
    let range = if show_graph {
        Some(CellRange::single(active))
    } else {
        cursor.range
    };
    if let Some((index, edit)) =
        super::point_strip::show(ui, "idle curve points", coordinates, &values, active, range)
    {
        selection = Some(Selection { index, edit });
    }
    Ok(selection)
}

fn show_plot(
    ui: &mut Ui,
    image: &mut Image,
    selected: usize,
    values: &mut [f64],
) -> Result<Option<Selection>, String> {
    let parameter = Parameter::Idle;
    let count = values.len();
    let (min, max) = theme::value_range(values);
    let padding = ((max - min) * 0.10).max(50.0);
    let size = vec2(
        ui.available_width(),
        (ui.available_height() - 102.0).clamp(200.0, 400.0),
    );
    let (response, painter) = ui.allocate_painter(size, Sense::click());
    painter.rect_filled(response.rect, 7, BG);
    let plot = eframe::egui::Rect::from_min_max(
        response.rect.min + vec2(58.0, 32.0),
        response.rect.max - vec2(24.0, 39.0),
    );
    let to_screen = |index: usize, value: f64| {
        pos2(
            egui::lerp(plot.x_range(), index as f32 / (count - 1) as f32),
            egui::lerp(
                plot.bottom()..=plot.top(),
                theme::normalize(value, min - padding, max + padding),
            ),
        )
    };
    let small = FontId::monospace(11.0);
    for i in 0..=5 {
        let value = egui::lerp((min - padding)..=(max + padding), f64::from(i) / 5.0);
        let left = to_screen(0, value);
        painter.line_segment(
            [left, pos2(plot.right(), left.y)],
            Stroke::new(1.0_f32, LINE),
        );
        painter.text(
            left - vec2(7.0, 0.0),
            Align2::RIGHT_CENTER,
            labels::format_table_value(value),
            small.clone(),
            MUTED,
        );
    }
    for i in [0, 8, 16, 24, 32, 38] {
        let x = to_screen(i, min).x;
        painter.line_segment(
            [pos2(x, plot.top()), pos2(x, plot.bottom())],
            Stroke::new(1.0_f32, LINE),
        );
        painter.text(
            pos2(x, plot.bottom() + 14.0),
            Align2::CENTER_CENTER,
            idle_temperature(i).expect("idle node"),
            small.clone(),
            MUTED,
        );
    }
    painter.text(
        response.rect.min + vec2(13.0, 12.0),
        Align2::LEFT_TOP,
        "об/мин",
        small.clone(),
        MUTED,
    );
    painter.text(
        response.rect.right_bottom() - vec2(11.0, 12.0),
        Align2::RIGHT_BOTTOM,
        "°C",
        small.clone(),
        MUTED,
    );
    let mut points: Vec<Pos2> = values
        .iter()
        .enumerate()
        .map(|(index, &value)| to_screen(index, value))
        .collect();
    let spec = parameter.spec();
    let mut displayed = values[selected];
    let mut moved = false;
    if parameter.writable()
        && plot.is_positive()
        && let Some(drag) = drag_point(
            ui,
            response.id.with(("curve point", selected)),
            points[selected],
            displayed,
            (max - min + 2.0 * padding) / f64::from(plot.height()),
            (spec.min, spec.max),
        )
    {
        displayed = drag.value;
        if drag.released {
            image
                .set(parameter, selected, displayed)
                .map_err(|error| error.to_string())?;
            displayed = super::current(image, parameter, selected);
            values[selected] = displayed;
            moved = true;
            ui.ctx().request_repaint();
        }
        points[selected].y = to_screen(selected, displayed)
            .y
            .clamp(plot.top(), plot.bottom());
    }
    painter.add(Shape::line(points.clone(), Stroke::new(2.3_f32, MINT)));
    for (index, point) in points.iter().enumerate() {
        painter.circle_filled(
            *point,
            if index == selected { 5.0 } else { 2.6 },
            if index == selected {
                SELECTED_CELL
            } else {
                MINT
            },
        );
    }
    let current = points[selected];
    painter.line_segment(
        [current, pos2(current.x, plot.bottom())],
        Stroke::new(1.0_f32, SELECTED_CELL),
    );
    painter.text(
        current - vec2(0.0, 9.0),
        Align2::CENTER_BOTTOM,
        labels::format_table_value(displayed),
        small,
        SELECTED_CELL,
    );

    let selection = response
        .interact_pointer_pos()
        .filter(|_| response.clicked() || response.double_clicked())
        .filter(|position| plot.expand(10.0).contains(*position))
        .map(|position| Selection {
            index: (((position.x - plot.left()) / plot.width()) * (values.len() - 1) as f32)
                .round()
                .clamp(0.0, (values.len() - 1) as f32) as usize,
            edit: response.double_clicked(),
        });

    Ok(selection.or(moved.then_some(Selection {
        index: selected,
        edit: false,
    })))
}
