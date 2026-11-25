//! Native regression check for the AppKit IME callback during window teardown.

#[cfg(target_os = "macos")]
#[path = "../src/macos.rs"]
mod macos;

#[cfg(target_os = "macos")]
mod smoke {
    use eframe::egui;
    use objc2::msg_send;
    use objc2::rc::Retained;
    use objc2_app_kit::NSView;
    use objc2_foundation::{NSRange, NSRect};
    use raw_window_handle::{HasWindowHandle, RawWindowHandle};
    use std::{cell::RefCell, ptr};

    thread_local! {
        static VIEW: RefCell<Option<Retained<NSView>>> = const { RefCell::new(None) };
    }

    struct Close;
    impl eframe::App for Close {
        fn update(&mut self, context: &egui::Context, _: &mut eframe::Frame) {
            context.send_viewport_cmd(egui::ViewportCommand::Close);
        }
    }

    fn query(view: &NSView) -> NSRect {
        unsafe {
            msg_send![view, firstRectForCharacterRange: NSRange::new(0, 0), actualRange: ptr::null_mut::<NSRange>()]
        }
    }

    pub fn run() -> eframe::Result {
        objc2::rc::autoreleasepool(|_| {
            eframe::run_native(
                "Janvar macOS text input smoke test",
                eframe::NativeOptions::default(),
                Box::new(|context| {
                    crate::macos::install_text_input_guard(context);
                    let RawWindowHandle::AppKit(handle) = context.window_handle()?.as_raw() else {
                        unreachable!();
                    };
                    let view = unsafe { handle.ns_view.cast::<NSView>().as_ref() };
                    let window = view.window().expect("attached window");
                    let content = window.contentView().expect("window content");
                    assert!(std::ptr::eq(view, &*content));
                    assert_ne!(query(view), NSRect::ZERO);
                    VIEW.with_borrow_mut(|saved| *saved = Some(content));
                    Ok(Box::new(Close))
                }),
            )
        })?;
        VIEW.with_borrow(|saved| {
            let view = saved.as_ref().unwrap();
            assert!(view.window().is_none());
            assert_eq!(query(view), NSRect::ZERO);
        });
        println!("AppKit text rectangles verified before and after native window teardown");
        Ok(())
    }
}

fn main() -> eframe::Result {
    #[cfg(target_os = "macos")]
    smoke::run()?;
    #[cfg(not(target_os = "macos"))]
    println!("This check requires macOS.");
    Ok(())
}
