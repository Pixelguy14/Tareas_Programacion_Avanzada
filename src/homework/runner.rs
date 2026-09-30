use std::process::Command;
use std::path::Path;

pub fn compile_and_run(src: &Path) -> Result<String, String> {
    // 1. Detectar si es C o C++
    let is_cpp = src.extension().and_then(|s| s.to_str()) == Some("cpp");
    let compiler = if is_cpp { "g++" } else { "gcc" };
    
    // Ruta temporal para el binario compilado
    let out_bin = "/tmp/homework_out.bin";

    // 2. Usar el compilador correcto (g++ para .cpp)
    let compile_status = Command::new(compiler)
        .arg("-O2")
        .arg("-march=native")
        .arg(src)
        .arg("-o")
        .arg(out_bin)
        .output()
        .map_err(|e| format!("Error ejecutando el compilador: {}", e))?;

    if !compile_status.status.success() {
        let err_msg = String::from_utf8_lossy(&compile_status.stderr);
        return Err(format!("Error de compilación ({}):\n{}", compiler, err_msg));
    }

    // 3. Ejecutar el binario generado
    let run_status = Command::new(out_bin)
        .output()
        .map_err(|e| format!("Error al ejecutar el binario: {}", e))?;

    if !run_status.status.success() {
        let err_msg = String::from_utf8_lossy(&run_status.stderr);
        return Err(format!("Error de ejecución:\n{}", err_msg));
    }

    Ok(String::from_utf8_lossy(&run_status.stdout).into_owned())
}