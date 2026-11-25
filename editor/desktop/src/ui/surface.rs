use super::theme::{self, ACCENT, BG, LINE, MUTED};
use super::{Selection, table};
use eframe::egui::epaint::{Mesh, Vertex, WHITE_UV};
use eframe::egui::{Align2, Color32, FontId, Pos2, Sense, Shape, Stroke, Ui, vec2};
use janvar_editor::firmware::{Image, Parameter};

pub struct Surface {
    azimuth: f32,
    elevation: f32,
}

impl Default for Surface {
    fn default() -> Self {
        Self {
            azimuth: -0.55,
            elevation: 0.64,
        }
    }
}

struct Projector {
    center: Pos2,
    scale: f32,
    azimuth: (f32, f32),
    elevation: (f32, f32),
}

impl Projector {
    fn point(&self, x: f32, y: f32, z: f32) -> (Pos2, f32) {
        let (sin_a, cos_a) = self.azimuth;
        let (sin_e, cos_e) = self.elevation;
        let right = x * cos_a - y * sin_a;
        let depth = x * sin_a + y * cos_a;
        let up = depth * sin_e - z * cos_e;
        (
            self.center + vec2(right, up) * self.scale,
            depth * cos_e + z * sin_e,
        )
    }
}

impl Surface {
    pub fn show(
        &mut self,
        ui: &mut Ui,
        image: &Image,
        parameter: Parameter,
        selected: usize,
    ) -> Option<Selection> {
        let values: Vec<_> = (0..parameter.count())
            .map(|index| super::current(image, parameter, index))
            .collect();
        let selection = self.show_values(
            ui,
            [16, 16],
            &values,
            selected,
            [
                "об/мин",
                if parameter == Parameter::Ignition {
                    "мг/цикл"
                } else {
                    "строка"
                },
                parameter.spec().unit,
            ],
        );
        let (min, max) = theme::value_range(&values);
        table::legend(ui, parameter, min, max);
        selection
    }

    pub fn show_values(
        &mut self,
        ui: &mut Ui,
        shape: [usize; 2],
        values: &[f64],
        selected: usize,
        axes: [&str; 3],
    ) -> Option<Selection> {
        let [columns, rows] = shape;
        ui.horizontal(|ui| {
            ui.label(theme::caption("Поворот перетаскиванием · щелчок по узлу"));
            if ui.button("Сбросить вид").clicked() {
                *self = Self::default();
            }
        });
        let (min, max) = theme::value_range(values);
        let dimensions = vec2(
            ui.available_width(),
            (ui.available_height() - 40.0).clamp(230.0, 540.0),
        );
        let (response, painter) = ui.allocate_painter(dimensions, Sense::click_and_drag());
        if response.dragged() {
            let motion = response.drag_delta();
            self.azimuth += motion.x * 0.009;
            self.elevation = (self.elevation - motion.y * 0.008).clamp(0.19, 1.48);
        }
        painter.rect_filled(response.rect, 7, BG);
        let project = Projector {
            center: response.rect.center() + vec2(0.0, dimensions.y * 0.12),
            scale: (dimensions.x * 0.64).min(dimensions.y * 0.73),
            azimuth: self.azimuth.sin_cos(),
            elevation: self.elevation.sin_cos(),
        };
        for n in 0..=5 {
            let tick = n as f32 / 5.0 - 0.5;
            for (a, b) in [
                (
                    project.point(tick, -0.5, 0.0),
                    project.point(tick, 0.5, 0.0),
                ),
                (
                    project.point(-0.5, tick, 0.0),
                    project.point(0.5, tick, 0.0),
                ),
            ] {
                painter.line_segment([a.0, b.0], Stroke::new(1.0_f32, LINE));
            }
        }
        let origin = project.point(-0.5, -0.5, 0.0).0;
        painter.line_segment(
            [origin, project.point(-0.5, -0.5, 0.64).0],
            Stroke::new(1.0_f32, MUTED),
        );
        let axis_font = FontId::proportional(12.0);
        for (pos, name) in [
            (project.point(0.0, 0.62, 0.0).0, axes[0]),
            (project.point(0.62, 0.0, 0.0).0, axes[1]),
            (project.point(-0.5, -0.5, 0.69).0, axes[2]),
        ] {
            painter.text(pos, Align2::CENTER_CENTER, name, axis_font.clone(), MUTED);
        }

        let nodes: Vec<_> = values
            .iter()
            .enumerate()
            .map(|(index, value)| {
                let t = theme::normalize(*value, min, max);
                let pos = node_position(&project, shape, index, t);
                let (pos, depth) = pos;
                (pos, depth, theme::heat(t))
            })
            .collect();
        let mut quads = Vec::with_capacity((columns - 1) * (rows - 1));
        for row in 0..rows - 1 {
            for column in 0..columns - 1 {
                let i = row * columns + column;
                let corners = [i, i + 1, i + columns + 1, i + columns];
                let depth: f32 = corners.iter().map(|&n| nodes[n].1).sum();
                quads.push((depth, corners));
            }
        }
        quads.sort_by(|left, right| left.0.total_cmp(&right.0));

        let mut mesh = Mesh::default();
        for (_, corners) in &quads {
            let base = mesh.vertices.len() as u32;
            for &corner in corners {
                let (pos, _, color) = nodes[corner];
                mesh.vertices.push(Vertex {
                    pos,
                    uv: WHITE_UV,
                    color,
                });
            }
            mesh.add_triangle(base, base + 1, base + 2);
            mesh.add_triangle(base, base + 2, base + 3);
        }
        painter.add(Shape::mesh(mesh));
        for (_, corners) in quads {
            let outline = corners.map(|corner| nodes[corner].0);
            painter.add(Shape::closed_line(
                outline.to_vec(),
                Stroke::new(0.6_f32, Color32::from_black_alpha(65)),
            ));
        }
        let point = nodes[selected].0;
        painter.circle_filled(point, 4.7, BG);
        painter.circle_stroke(point, 4.7, Stroke::new(2.0_f32, ACCENT));

        let click = response.clicked() || response.double_clicked();
        let pointer = response.interact_pointer_pos().filter(|_| click)?;
        let index = hit_node(nodes.iter().map(|node| node.0), pointer)?;
        Some(Selection {
            index,
            edit: response.double_clicked(),
        })
    }
}

fn node_position(
    projector: &Projector,
    [columns, rows]: [usize; 2],
    index: usize,
    height: f32,
) -> (Pos2, f32) {
    projector.point(
        (index % columns) as f32 / (columns - 1).max(1) as f32 - 0.5,
        (index / columns) as f32 / (rows - 1).max(1) as f32 - 0.5,
        0.07 + 0.55 * height,
    )
}

fn hit_node(positions: impl Iterator<Item = Pos2>, pointer: Pos2) -> Option<usize> {
    let (index, distance) = positions
        .enumerate()
        .map(|(index, point)| (index, point.distance_sq(pointer)))
        .min_by(|a, b| a.1.total_cmp(&b.1))?;
    (distance <= 225.0).then_some(index)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::demo::SAMPLE;
    use janvar_editor::firmware::TableCell;

    #[test]
    fn projected_point_selects_the_same_node_for_editing_and_reopening() {
        let parameter = Parameter::Ignition;
        let mut image = Image::new(SAMPLE.to_vec()).unwrap();
        let projector = Projector {
            center: Pos2::ZERO,
            scale: 300.0,
            azimuth: (-0.55_f32).sin_cos(),
            elevation: 0.64_f32.sin_cos(),
        };
        let values: Vec<_> = (0..parameter.count())
            .map(|index| image.value(parameter, index).unwrap())
            .collect();
        let (min, max) = theme::value_range(&values);
        let points: Vec<_> = values
            .iter()
            .enumerate()
            .map(|(index, &value)| {
                node_position(
                    &projector,
                    [16, 16],
                    index,
                    theme::normalize(value, min, max),
                )
                .0
            })
            .collect();
        let chosen = TableCell::new(1, 1, parameter).unwrap().index();
        let selected = hit_node(points.iter().copied(), points[chosen]).unwrap();
        assert_eq!(selected, chosen);
        assert_eq!(
            hit_node(points.into_iter(), Pos2::new(10000.0, 10000.0)),
            None
        );
        image.set(parameter, selected, -25.5).unwrap();

        let decoded = Image::new(image.to_bin()).unwrap();
        assert_eq!(decoded.value(parameter, chosen), Some(-25.5));
        assert_eq!(decoded.bytes()[0x6AD9], 0xCD);
    }
}
