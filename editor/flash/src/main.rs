#[cfg(windows)]
mod windows;

fn main() -> std::process::ExitCode {
    #[cfg(windows)]
    {
        match windows::run() {
            Ok(()) => std::process::ExitCode::SUCCESS,
            Err(error) => {
                eprintln!("{error}");
                std::process::ExitCode::FAILURE
            }
        }
    }

    #[cfg(not(windows))]
    {
        eprintln!("janvar-flash requires Windows.");
        std::process::ExitCode::FAILURE
    }
}
