# Programación Avanzada — Tareas GUI

> **Cinvestav CS 2026-1 · Unidad 1**  
> GUI interactiva para compilar y ejecutar las tareas del curso, con visualización de estructuras de datos.

---

## Descripción

Este proyecto es una aplicación de terminal (TUI) construida con [ratatui](https://ratatui.rs/) que actúa como menú de tareas.  
Permite:

- **Seleccionar** cualquier tarea del menú con las flechas del teclado.
- **Compilar automáticamente** el código C usando `gcc` desde Rust (vía `std::process::Command`).
- **Ver la salida** del programa directamente en la terminal.
- **Visualizar estructuras de datos** 

El código C de cada tarea también se puede compilar y ejecutar **de forma independiente** con `gcc` sin necesidad de Rust.

---

## Requisitos

| Herramienta | Versión mínima | Comando de verificación |
|-------------|---------------|-------------------------|
| Rust + Cargo | 1.75+ | `cargo --version` |
| GCC | 12+ | `gcc --version` |
| Terminal | 256 colores | — |

### Instalar Rust

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

---

## Ejecutar la GUI

```bash
# Desde dentro de Tareas/
cargo run
```

O en modo release (más rápido):

```bash
cargo run --release
```

### Controles del teclado

| Pantalla | Tecla | Acción |
|----------|-------|--------|
| Menú | `↑` / `↓` ó `k` / `j` | Navegar tareas |
| Menú | `Enter` | Compilar y ejecutar |
| Menú | `q` | Salir |
| Salida / Error | `Esc` ó `q` | Volver al menú |
| Visualización | `←` / `→` ó `h` / `l` | Paso anterior / siguiente |
| Visualización | `Home` / `End` | Primer / último paso |
| Visualización | `Esc` ó `q` | Volver al menú |

---

## Compilar tareas C de forma independiente

Cada archivo `.c` en `c_src/` es autónomo y se puede compilar directamente:

```bash
# Tarea 1 — multiplicación de 128 bits
gcc -O2 -march=native -o Tarea1.bin c_src/Tarea1.c && ./Tarea1.bin

# Tarea 2 - LIFO y FIFO en lista doblemente enlazada
gcc -o c_src/Tarea2.o c_src/Tarea2.c

# Tarea 3 - Calculo de horner intrinsics usando precision simple
g++ -O2 -march=native Tarea3.cpp -o Tarea3.bin
```

---

## Agregar una nueva tarea

1. Coloca el archivo `.c` en `c_src/TareaN.c`.
2. Agrega una entrada en `src/homework/mod.rs` dentro de la función `registry()`:

```rust
HomeworkEntry {
    label: "TareaN — Descripción breve".into(),
    src:   PathBuf::from("c_src/TareaN.c"),
    description: "Descripción larga mostrada en el panel lateral.".into(),
    viz:   VizKind::PlainText,   // o Stack, Queue, …
},
```

3. Ejecuta `cargo run` — para mostrar las tareas en el menú automáticamente.

---

## ⚠️ Nota para la red de Cinvestav

La red institucional de Cinvestav utiliza un sistema de filtrado (Fortiguard) que **bloquea el acceso a `crates.io`**, el registro de paquetes de Rust.

---

## Contenido temático del curso

1. **Conceptos básicos**
   - 1.1 Proceso de desarrollo de un programa
   - 1.2 Tipos de datos abstractos
   - 1.3 Organización de código
   - 1.4 Funciones
   - 1.5 Punteros y manejo de memoria
   - 1.6 Fugas de memoria

2. **Estructuras de datos**
   - 2.1 Pilas, colas, listas, listas doblemente ligadas
   - 2.2 Árboles: concepto, árboles binarios, recorridos, balanceo, búsqueda
   - 2.3 Algoritmos de búsqueda, ordenamiento y hash

3. **STL (Standard Template Library)**
   - 3.1 Vectores
   - 3.2 Iteradores
   - 3.3 Vectores bidimensionales
   - 3.4 Colas y listas