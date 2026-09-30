pub mod runner;

use std::path::PathBuf;

/// A homework entry shown in the menu.
#[derive(Debug, Clone)]
pub struct HomeworkEntry {
    /// Display label (e.g. "Tarea1 — 128-bit multiplication")
    pub label: String,
    /// Path to the .c source file relative to the project root.
    pub src: PathBuf,
    /// Optional description shown in the sidebar.
    pub description: String,
    /// Kind of visualization to apply to the output.
    pub viz: VizKind,
}

/// What kind of visualization to apply to the program output.
#[derive(Debug, Clone, PartialEq)]
pub enum VizKind {
    /// Plain text output — just display stdout.
    PlainText,
    /// Parse PUSH/POP lines and render a stack widget.
    Stack,
    /// Parse ENQUEUE/DEQUEUE lines and render a queue widget.
    Queue,
    // Future: Plot, Tree, LinkedList, …
}

/// Hard-coded homework registry.
pub fn registry() -> Vec<HomeworkEntry> {
    vec![
        HomeworkEntry {
            label: "Tarea1 — Multiplicación de 128 bits con intrinsics".into(),
            src: PathBuf::from("c_src/Tarea1.c"),
            description: "Implementar multiplicación de 128 bits usando uniones y _mulx_u64.\n\
                           Temas: struct/union, Intel intrinsics (SIMD), alineación de memoria."
                .into(),
            viz: VizKind::PlainText,
        },
        HomeworkEntry {
            label: "LIFO y FIFO en listas doblemente enlazadas".into(),
            src: PathBuf::from("c_src/Tarea2.c"),
            description: "Implementar funciones de estructura LIFO y FIFO para listas en C.\n\
                           Temas: estructuras de datos, pilas, colas, manejo de índices."
                .into(),
            viz: VizKind::PlainText,
        },
        HomeworkEntry {
            label: "Calculo de horner intrinsics usando precision simple".into(),
            src: PathBuf::from("c_src/Tarea3.cpp"),
            description: "Implementar el programa original de clase reemplazando la lógica de Double a Float.\n\
                           Temas: intrinsics, benchmarking, optimización de funciones."
                .into(),
            viz: VizKind::PlainText,
        },
    ]
}
