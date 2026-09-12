use std::{path::Path, process::Command};

/// Compile `src_path` with gcc and run the resulting binary.
/// Returns `Ok(stdout)` or `Err(error_message)`.
pub fn compile_and_run(src_path: &Path) -> Result<String, String> {
    // Derive binary path: replace .c extension with .bin inside c_src/
    let bin_path = src_path.with_extension("bin");

    // ── Compile ──────────────────────────────────────────────────────────────
    let compile = Command::new("gcc")
        .args([
            src_path.to_str().unwrap(),
            "-o",
            bin_path.to_str().unwrap(),
            "-march=native",
            "-O2",
            "-lm",
        ])
        .output()
        .map_err(|e| format!("No se pudo invocar gcc: {e}"))?;

    if !compile.status.success() {
        let stderr = String::from_utf8_lossy(&compile.stderr);
        return Err(format!("Error de compilación (gcc):\n{stderr}"));
    }

    // ── Run ──────────────────────────────────────────────────────────────────
    let run = Command::new(&bin_path)
        .output()
        .map_err(|e| format!("No se pudo ejecutar el binario: {e}"))?;

    let stdout = String::from_utf8_lossy(&run.stdout).to_string();
    let stderr = String::from_utf8_lossy(&run.stderr);

    if !run.status.success() {
        return Err(format!("El programa terminó con error:\n{stderr}\nSalida:\n{stdout}"));
    }

    Ok(stdout)
}
