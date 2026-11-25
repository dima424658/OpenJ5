use super::cells::CellRange;
use super::labels::format_table_value;
use super::table;
use super::theme::{self, BG, SELECTED_CELL};
use eframe::egui::{
    self, Align, FontSelection, Layout, RichText, TextWrapMode, Ui, WidgetText, vec2,
};

/// Show the X coordinate above the Y value for every point of a curve.
pub fn show(
    ui: &mut Ui,
    id: impl std::hash::Hash,
    coordinates: impl IntoIterator<Item = String>,
    values: &[f64],
    selected: usize,
    range: Option<CellRange>,
) -> Option<(usize, bool)> {
    let selection_id = ui.make_persistent_id(("selected curve node", &id));
    let reveal_selected = ui.data_mut(|data| {
        let changed = data.get_temp::<usize>(selection_id) != Some(selected);
        data.insert_temp(selection_id, selected);
        changed
    });
    let measure = |text: RichText| {
        WidgetText::from(text).into_galley(
            ui,
            Some(TextWrapMode::Extend),
            f32::INFINITY,
            FontSelection::Default,
        )
    };
    let (min, max) = theme::value_range(values);
    let mut width: f32 = 64.0;
    let padding = ui.spacing().button_padding.x * 2.0;
    let points: Vec<_> = coordinates
        .into_iter()
        .zip(values)
        .enumerate()
        .map(|(index, (x, &y))| {
            let active = range.is_some_and(|range| range.contains(index, values.len()));
            let coordinate = measure(table::axis_text(x, active));
            let value = measure(RichText::new(format_table_value(y)).monospace().color(BG));
            width = width.max(coordinate.size().x.max(value.size().x) + padding);
            (
                coordinate,
                value,
                theme::heat(theme::normalize(y, min, max)),
            )
        })
        .collect();
    let width = width.ceil();
    egui::ScrollArea::horizontal()
        .id_salt(id)
        .show(ui, |ui| {
            let mut selection = None;
            ui.horizontal_top(|ui| {
                ui.spacing_mut().item_spacing.x = 4.0;
                for (index, (coordinate, value, color)) in points.into_iter().enumerate() {
                    let active = range.is_some_and(|range| range.contains(index, values.len()));
                    let response = ui
                        .allocate_ui_with_layout(
                            vec2(width, 0.0),
                            Layout::top_down(Align::Center),
                            |ui| {
                                ui.spacing_mut().item_spacing.y = 3.0;
                                table::axis_cell(ui, coordinate, active, width);
                                ui.add_sized(
                                    [width, 28.0],
                                    egui::Button::new(value).fill(if active {
                                        SELECTED_CELL
                                    } else {
                                        color
                                    }),
                                )
                            },
                        )
                        .inner;
                    if index == selected
                        && reveal_selected
                        && !ui.clip_rect().contains_rect(response.rect)
                    {
                        response.scroll_to_me(Some(Align::Center));
                    }
                    if response.clicked() || response.double_clicked() {
                        selection = Some((index, response.double_clicked()));
                    }
                }
            });
            selection
        })
        .inner
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ui::curve_plot::CurvePlot;

    #[test]
    fn curve_labels_round_and_colored_cells_fit_on_one_line() {
        let context = egui::Context::default();
        theme::install(&context);
        let values = [-123_456.753, 321.416313, 999.997];
        let output = context.run(egui::RawInput::default(), |ctx| {
            egui::CentralPanel::default().show(ctx, |ui| {
                CurvePlot {
                    x: &[-40.0, -35.0, -30.0],
                    y: &values,
                    labels: ["°C", "мг/цикл"],
                    bounds: (values[0], values[2]),
                }
                .show(ui, 1, None);
                show(
                    ui,
                    "curve points",
                    ["-40 °C", "-35 °C", "-30 °C"].map(str::to_owned),
                    &values,
                    1,
                    Some(CellRange::single(1)),
                );
            });
        });

        let colors = [theme::heat(0.0), SELECTED_CELL, theme::heat(1.0)];
        let cells: Vec<_> = output
            .shapes
            .iter()
            .filter_map(|shape| match &shape.shape {
                egui::Shape::Rect(rect) if colors.contains(&rect.fill) => Some(rect),
                _ => None,
            })
            .collect();
        assert_eq!(cells.len(), 3);
        for (cell, color) in cells.iter().zip(colors) {
            assert_eq!(cell.fill, color);
            assert_eq!(cell.rect.size(), cells[0].rect.size());
        }
        for pair in cells.windows(2) {
            assert_eq!(pair[1].rect.left() - pair[0].rect.right(), 4.0);
        }
        for (cell, label) in cells.iter().zip(["-40 °C", "-35 °C", "-30 °C"]) {
            let header = output
                .shapes
                .iter()
                .find_map(|shape| match &shape.shape {
                    egui::Shape::Text(text) if text.galley.text() == label => Some(text),
                    _ => None,
                })
                .unwrap();
            let center = header.pos.x + header.galley.size().x * 0.5;
            assert!((center - cell.rect.center().x).abs() < 1.0);
            assert!(header.pos.y < cell.rect.top());
        }

        let text: Vec<_> = output
            .shapes
            .iter()
            .filter_map(|shape| match &shape.shape {
                egui::Shape::Text(text) => Some(&text.galley),
                _ => None,
            })
            .collect();
        assert_eq!(
            text.iter()
                .filter(|galley| galley.text() == "321.42")
                .count(),
            2
        );
        for label in ["-123456.75", "321.42", "1000"] {
            let galley = text
                .iter()
                .rev()
                .find(|galley| galley.text() == label)
                .unwrap();
            assert_eq!(galley.rows.len(), 1);
            assert!(galley.size().x < cells[0].rect.width());
        }
    }
}
