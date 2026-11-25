use super::labels::format_table_value;
use super::theme::{self, BG, LINE, MINT, MUTED, SELECTED_CELL};
use eframe::egui::{
    Align2, CursorIcon, FontId, Id, PointerButton, Pos2, Rect, Sense, Stroke, Ui, pos2, vec2,
};

pub enum CurveAction {
    Select(usize),
    SetValue(f64),
}

pub struct PointDrag {
    pub value: f64,
    pub released: bool,
}

pub fn drag_point(
    ui: &mut Ui,
    id: Id,
    point: Pos2,
    current: f64,
    units_per_pixel: f64,
    limits: (f64, f64),
) -> Option<PointDrag> {
    let handle = ui
        .interact(
            Rect::from_center_size(point, vec2(16.0, 16.0)),
            id,
            Sense::drag(),
        )
        .on_hover_and_drag_cursor(CursorIcon::ResizeVertical);
    if handle.drag_started_by(PointerButton::Primary) {
        let origin = ui.input(|input| input.pointer.press_origin())?;
        ui.data_mut(|data| data.insert_temp(id, origin.y));
    }
    let released = handle.drag_stopped_by(PointerButton::Primary);
    if !released && !handle.dragged_by(PointerButton::Primary) {
        return None;
    }
    let origin_y = ui.data_mut(|data| {
        if released {
            data.remove_temp::<f32>(id)
        } else {
            data.get_temp::<f32>(id)
        }
    })?;
    let pointer = handle.interact_pointer_pos()?;
    Some(PointDrag {
        value: (current + f64::from(origin_y - pointer.y) * units_per_pixel)
            .clamp(limits.0, limits.1),
        released,
    })
}

pub struct CurvePlot<'a> {
    pub x: &'a [f64],
    pub y: &'a [f64],
    pub labels: [&'a str; 2],
    pub bounds: (f64, f64),
}

impl CurvePlot<'_> {
    pub fn show(
        &self,
        ui: &mut Ui,
        selected: usize,
        edit_bounds: Option<(f64, f64)>,
    ) -> Option<CurveAction> {
        let size = vec2(
            ui.available_width(),
            (ui.available_height() - 86.0).clamp(235.0, 520.0),
        );
        let (response, painter) = ui.allocate_painter(size, Sense::click());
        painter.rect_filled(response.rect, 6.0, BG);
        let rect = Rect::from_min_max(
            response.rect.min + vec2(58.0, 40.0),
            response.rect.max - vec2(21.0, 48.0),
        );
        if !rect.is_positive() {
            return None;
        }
        let font = FontId::monospace(11.0);
        let (x0, x1) = (self.x[0], self.x[self.x.len() - 1]);
        let (y0, y1) = self.bounds;
        let px = |value| rect.left() + rect.width() * theme::normalize(value, x0, x1);
        let py = |value| rect.bottom() - rect.height() * theme::normalize(value, y0, y1);

        for value in ticks(y0, y1, (rect.height() / 36.0).clamp(3.0, 12.0) as usize) {
            let y = py(value);
            painter.hline(rect.x_range(), y, Stroke::new(1.0_f32, LINE));
            painter.text(
                pos2(rect.left() - 7.0, y),
                Align2::RIGHT_CENTER,
                format_table_value(value),
                font.clone(),
                MUTED,
            );
        }
        let stride = (self.x.len() - 1).div_ceil((rect.width() / 48.0).max(1.0) as usize);
        let last = self.x.len() - 1;
        let last_label = painter.layout_no_wrap(format_table_value(x1), font.clone(), MUTED);
        let last_left = px(x1) - last_label.size().x * 0.5;
        let mut next_label_left = f32::NEG_INFINITY;
        for i in (0..self.x.len())
            .step_by(stride)
            .filter(|&i| i == 0 || i + stride.div_ceil(2) < self.x.len())
            .chain(std::iter::once(last))
        {
            let x = px(self.x[i]);
            painter.vline(x, rect.y_range(), Stroke::new(1.0_f32, LINE));
            let label = if i == last {
                last_label.clone()
            } else {
                painter.layout_no_wrap(format_table_value(self.x[i]), font.clone(), MUTED)
            };
            let label_rect =
                Align2::CENTER_TOP.anchor_size(pos2(x, rect.bottom() + 10.0), label.size());
            if label_rect.left() >= next_label_left
                && (i == last || label_rect.right() + 6.0 <= last_left)
            {
                next_label_left = label_rect.right() + 6.0;
                painter.galley(label_rect.min, label, MUTED);
            }
        }
        painter.text(
            pos2(rect.left(), rect.top() - 25.0),
            Align2::LEFT_TOP,
            self.labels[1],
            FontId::proportional(12.0),
            MUTED,
        );
        painter.text(
            pos2(rect.center().x, rect.bottom() + 30.0),
            Align2::CENTER_TOP,
            self.labels[0],
            FontId::proportional(12.0),
            MUTED,
        );

        let mut points: Vec<_> = self
            .x
            .iter()
            .zip(self.y)
            .map(|(&x, &y)| pos2(px(x), py(y)))
            .collect();
        let drag = edit_bounds.and_then(|limits| {
            drag_point(
                ui,
                response.id.with(("curve point", selected)),
                points[selected],
                self.y[selected],
                (y1 - y0) / f64::from(rect.height()),
                limits,
            )
        });
        let value = drag.as_ref().map_or(self.y[selected], |drag| drag.value);
        points[selected].y = py(value).clamp(rect.top(), rect.bottom());
        painter.add(eframe::egui::Shape::line(
            points.clone(),
            Stroke::new(1.8_f32, MINT),
        ));
        if rect.width() / points.len() as f32 >= 5.0 {
            for &point in &points {
                painter.circle_filled(point, 2.2, MINT);
            }
        }
        let point = points[selected];
        painter.circle_filled(point, 4.8, SELECTED_CELL);
        painter.text(
            point - vec2(0.0, 8.0),
            Align2::CENTER_BOTTOM,
            format_table_value(value),
            font,
            SELECTED_CELL,
        );
        if let Some(drag) = drag.filter(|drag| drag.released) {
            return Some(CurveAction::SetValue(drag.value));
        }
        response
            .interact_pointer_pos()
            .filter(|pointer| response.clicked() && rect.expand(6.0).contains(*pointer))
            .and_then(|pointer| {
                points
                    .iter()
                    .enumerate()
                    .min_by(|(_, a), (_, b)| {
                        (a.x - pointer.x).abs().total_cmp(&(b.x - pointer.x).abs())
                    })
                    .map(|(i, _)| CurveAction::Select(i))
            })
    }
}

fn ticks(min: f64, max: f64, target: usize) -> impl Iterator<Item = f64> {
    let approximate = (max - min) / target as f64;
    let magnitude = 10.0_f64.powf(approximate.log10().floor());
    let step = [1.0, 2.0, 2.5, 5.0, 10.0]
        .into_iter()
        .map(|factor| factor * magnitude)
        .min_by(|a, b| {
            (*a - approximate)
                .abs()
                .total_cmp(&(*b - approximate).abs())
        })
        .unwrap_or(approximate);
    let first = (min / step).ceil();
    (0..=target * 3)
        .map(move |i| (first + i as f64) * step)
        .take_while(move |&value| value <= max)
}

#[cfg(test)]
mod tests {
    use super::*;
    use eframe::egui;

    #[test]
    fn axis_labels_use_two_decimals_without_horizontal_overlap() {
        let context = egui::Context::default();
        let x: Vec<_> = (0..32).map(|i| f64::from(i) * 255.0 / 31.0).collect();
        let output = context.run(
            egui::RawInput {
                screen_rect: Some(Rect::from_min_size(Pos2::ZERO, vec2(850.0, 480.0))),
                ..Default::default()
            },
            |context| {
                egui::CentralPanel::default().show(context, |ui| {
                    CurvePlot {
                        x: &x,
                        y: &[0.25; 32],
                        labels: ["X", "Y"],
                        bounds: (0.0, 0.123),
                    }
                    .show(ui, 0, None);
                });
            },
        );
        let labels: Vec<_> = output
            .shapes
            .iter()
            .filter_map(|shape| match &shape.shape {
                egui::Shape::Text(text) => Some(text),
                _ => None,
            })
            .collect();
        let text = |label: &str| labels.iter().find(|text| text.galley.text() == label);
        assert!(text("16.45").is_some() || text("24.68").is_some());
        assert!(text("0.02").is_some());
        let x_axis_y = text("255").unwrap().pos.y;
        let x_labels: Vec<_> = labels
            .iter()
            .filter(|text| text.pos.y == x_axis_y)
            .collect();
        assert!(x_labels.len() > 2);
        assert!(
            x_labels
                .windows(2)
                .all(|pair| { pair[0].pos.x + pair[0].galley.size().x <= pair[1].pos.x })
        );
        for label in labels {
            if let Some((_, decimals)) = label.galley.text().split_once('.') {
                assert!(decimals.len() <= 2);
            }
        }
    }
}
