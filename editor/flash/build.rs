use std::env;
use std::path::{Path, PathBuf};
use std::process::Command;

const FRAMES_ENV: &str = "JANVAR_BOOTSTRAP_FRAMES";

fn main() {
    println!("cargo:rerun-if-env-changed={FRAMES_ENV}");
    println!("cargo:rerun-if-env-changed=PYTHON");
    for source in [
        "bootstrap/build_c.py",
        "bootstrap/c509/bootstrap.c",
        "bootstrap/c509/bootstrap.h",
    ] {
        println!("cargo:rerun-if-changed={source}");
    }

    let frames = if let Some(path) = env::var_os(FRAMES_ENV) {
        let path = PathBuf::from(path);
        println!("cargo:rerun-if-changed={}", path.display());
        path
    } else {
        let out_dir = env::var_os("OUT_DIR").expect("Cargo must set OUT_DIR");
        build_bootstrap(Path::new(&out_dir))
    };
    println!("cargo:rustc-env={FRAMES_ENV}={}", frames.display());
}

fn build_bootstrap(out_dir: &Path) -> PathBuf {
    let frames = out_dir.join("combi-bootstrap-c-frames.bin");
    let default_python = if cfg!(windows) { "python" } else { "python3" };
    let python = env::var_os("PYTHON").unwrap_or_else(|| default_python.into());
    let status = Command::new(python)
        .arg("bootstrap/build_c.py")
        .arg(out_dir.join("bootstrap-c.ihx"))
        .arg("--frames")
        .arg(&frames)
        .status()
        .expect("could not run Python 3 to build the C509-L bootstrap");
    assert!(status.success(), "C509-L bootstrap build failed");
    frames
}
