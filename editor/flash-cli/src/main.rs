mod prepare;

#[cfg(any(windows, target_os = "macos", target_os = "linux"))]
mod cli;

fn main() -> std::process::ExitCode {
    let mut args = std::env::args_os().skip(1);
    if args
        .next()
        .is_some_and(|command| command == "prepare-flash")
    {
        return match prepare::run_prepare(args) {
            Ok(()) => std::process::ExitCode::SUCCESS,
            Err(error) => {
                eprintln!("{error}");
                std::process::ExitCode::FAILURE
            }
        };
    }

    #[cfg(any(windows, target_os = "macos", target_os = "linux"))]
    {
        match cli::run() {
            Ok(()) => std::process::ExitCode::SUCCESS,
            Err(error) => {
                eprintln!("{error}");
                std::process::ExitCode::FAILURE
            }
        }
    }

    #[cfg(not(any(windows, target_os = "macos", target_os = "linux")))]
    {
        eprintln!(
            "Serial-port operations require Windows, macOS, or Linux.\n{}",
            prepare::PREPARE_USAGE
        );
        std::process::ExitCode::FAILURE
    }
}
