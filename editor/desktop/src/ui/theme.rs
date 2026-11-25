use eframe::egui::{self, Color32, Frame, Margin, RichText, Stroke, TextStyle, Ui};

pub const BG: Color32 = Color32::from_rgb(16, 22, 30);
pub const PANEL: Color32 = Color32::from_rgb(23, 31, 42);
pub const CARD: Color32 = Color32::from_rgb(29, 39, 52);
pub const LINE: Color32 = Color32::from_rgb(48, 62, 79);
pub const MUTED: Color32 = Color32::from_rgb(158, 176, 198);
pub const TEXT: Color32 = Color32::from_rgb(228, 235, 244);
pub const ACCENT: Color32 = Color32::from_rgb(255, 190, 115);
pub const SELECTED_CELL: Color32 = Color32::from_rgb(100, 174, 248);
pub const MINT: Color32 = Color32::from_rgb(102, 223, 189);
pub const DANGER: Color32 = Color32::from_rgb(255, 145, 140);

pub fn install(ctx: &egui::Context) {
    let mut style = (*ctx.style()).clone();
    style.spacing.item_spacing = egui::vec2(8.0, 7.0);
    style.spacing.button_padding = egui::vec2(10.0, 5.0);
    style.spacing.interact_size.y = 28.0;
    style.text_styles.get_mut(&TextStyle::Body).unwrap().size = 14.0;
    let mut visual = egui::Visuals::dark();
    visual.override_text_color = Some(TEXT);
    visual.panel_fill = PANEL;
    visual.window_fill = PANEL;
    visual.window_stroke = Stroke::new(1.0_f32, LINE);
    visual.extreme_bg_color = BG;
    visual.faint_bg_color = CARD;
    visual.selection.bg_fill = Color32::from_rgb(52, 70, 86);
    visual.selection.stroke = Stroke::new(1.0_f32, ACCENT);
    visual.widgets.noninteractive.bg_stroke.color = LINE;
    visual.widgets.noninteractive.fg_stroke.color = MUTED;
    visual.widgets.inactive.weak_bg_fill = CARD;
    visual.widgets.inactive.bg_fill = CARD;
    visual.widgets.inactive.bg_stroke = Stroke::new(1.0_f32, LINE);
    visual.widgets.hovered.weak_bg_fill = Color32::from_rgb(43, 58, 75);
    visual.widgets.hovered.bg_fill = visual.widgets.hovered.weak_bg_fill;
    visual.widgets.hovered.bg_stroke = Stroke::new(1.0_f32, MUTED);
    visual.widgets.active.weak_bg_fill = visual.selection.bg_fill;
    visual.widgets.active.bg_fill = visual.selection.bg_fill;
    style.visuals = visual;
    ctx.set_style(style);
}

pub fn panel(color: Color32, margins: Margin) -> Frame {
    Frame::new().fill(color).inner_margin(margins)
}

pub fn card(ui: &mut Ui, content: impl FnOnce(&mut Ui)) {
    Frame::new()
        .fill(CARD)
        .stroke(Stroke::new(1.0_f32, LINE))
        .corner_radius(7)
        .inner_margin(Margin::same(12))
        .show(ui, content);
}

pub fn caption(text: impl Into<String>) -> RichText {
    RichText::new(text).size(12.0).color(MUTED)
}

pub fn primary(label: &str) -> egui::Button<'_> {
    egui::Button::new(RichText::new(label).strong().color(BG))
        .fill(ACCENT)
        .stroke(Stroke::NONE)
}

pub fn value_range(values: &[f64]) -> (f64, f64) {
    let (mut min, mut max) = (f64::INFINITY, f64::NEG_INFINITY);
    for &value in values {
        min = min.min(value);
        max = max.max(value);
    }
    (min, max)
}

pub fn normalize(value: f64, min: f64, max: f64) -> f32 {
    if min < max {
        ((value - min) / (max - min)) as f32
    } else {
        0.5
    }
}

pub fn heat(t: f32) -> Color32 {
    let colors = [
        Color32::from_rgb(240, 78, 75),
        Color32::from_rgb(247, 145, 55),
        Color32::from_rgb(245, 218, 85),
        Color32::from_rgb(204, 234, 176),
    ];
    let position = t.clamp(0.0, 1.0) * (colors.len() - 1) as f32;
    let index = (position.floor() as usize).min(colors.len() - 2);
    let mix = position - index as f32;
    let a = colors[index].to_array();
    let b = colors[index + 1].to_array();
    let blend =
        |channel| egui::lerp(f32::from(a[channel])..=f32::from(b[channel]), mix).round() as u8;
    Color32::from_rgb(blend(0), blend(1), blend(2))
}

pub fn status_dot(ui: &mut Ui, color: Color32) {
    let (response, painter) = ui.allocate_painter(egui::vec2(7.0, 7.0), egui::Sense::hover());
    painter.circle_filled(response.rect.center(), 3.0, color);
}
