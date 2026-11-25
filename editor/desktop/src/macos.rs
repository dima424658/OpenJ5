use objc2::runtime::{Imp, Sel};
use objc2::{ClassType, sel};
use objc2_app_kit::NSView;
use objc2_foundation::{NSRange, NSRect};
use raw_window_handle::{HasWindowHandle, RawWindowHandle};
use std::sync::OnceLock;

type TextRect = unsafe extern "C-unwind" fn(&NSView, Sel, NSRange, *mut NSRange) -> NSRect;

static ORIGINAL: OnceLock<Imp> = OnceLock::new();

/// Backports the detached-view guard from winit PR #4334 for the 0.30 AppKit backend.
pub fn install_text_input_guard(context: &eframe::CreationContext<'_>) {
    let Ok(handle) = context.window_handle() else {
        return;
    };
    let RawWindowHandle::AppKit(handle) = handle.as_raw() else {
        return;
    };

    // SAFETY: The creation context keeps the AppKit window and its NSView alive.
    let view = unsafe { handle.ns_view.cast::<NSView>().as_ref() };
    let class = view.as_super().as_super().class();
    if class.name() != "WinitView" {
        return;
    }
    let selector = sel!(firstRectForCharacterRange:actualRange:);
    let Some(method) = class.instance_method(selector) else {
        return;
    };

    ORIGINAL.get_or_init(|| {
        // SAFETY: WinitView's method uses the NSTextInputClient signature (TextRect).
        // Installation runs on AppKit's main thread before this callback can run again.
        unsafe { method.set_implementation(std::mem::transmute::<TextRect, Imp>(text_rect)) }
    });
}

unsafe extern "C-unwind" fn text_rect(
    view: &NSView,
    selector: Sel,
    range: NSRange,
    actual_range: *mut NSRange,
) -> NSRect {
    // Keep the window alive for the duration of the original callback.
    let Some(_window) = view.window() else {
        return NSRect::ZERO;
    };
    let Some(original) = ORIGINAL.get().copied() else {
        return NSRect::ZERO;
    };

    // SAFETY: The implementation was captured from this exact selector on WinitView.
    // The receiver and arguments come from AppKit and remain valid for this call.
    unsafe { std::mem::transmute::<Imp, TextRect>(original)(view, selector, range, actual_range) }
}
