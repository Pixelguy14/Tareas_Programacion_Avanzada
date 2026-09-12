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
- **Visualizar estructuras de datos** (pilas, colas) paso a paso con animación.

El código C de cada tarea también se puede compilar y ejecutar **de forma independiente** con `gcc` sin necesidad de Rust.

---

## Estructura del proyecto

```
Tareas/
├── Cargo.toml              # Proyecto Rust (tareas-gui)
├── .gitignore
├── README.md               # Este archivo
│
├── src/                    # Código fuente Rust (GUI)
│   ├── main.rs             # Punto de entrada
│   ├── app.rs              # Máquina de estados de la aplicación
│   ├── tui.rs              # Inicialización / restauración del terminal
│   ├── ui.rs               # Renderizado de pantallas (ratatui)
│   ├── homework/
│   │   ├── mod.rs          # Registro de tareas (HomeworkEntry)
│   │   └── runner.rs       # Compilar y ejecutar código C via gcc
│   └── viz/
│       ├── mod.rs          # Parsers de salida estructurada
│       ├── stack.rs        # Widget de pila (LIFO)
│       └── queue.rs        # Widget de cola (FIFO)
│
└── c_src/                  # Código fuente C (tareas y demos)
    ├── Tarea1.c            # Multiplicación de 128 bits con intrinsics
    ├── Test_Stack.c        # Demo: pila con visualización en GUI
    └── Test_Queue.c        # Demo: cola circular con visualización en GUI
```

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

# Demo pila
gcc -o Test_Stack.bin c_src/Test_Stack.c && ./Test_Stack.bin

# Demo cola
gcc -o Test_Queue.bin c_src/Test_Queue.c && ./Test_Queue.bin
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

3. Ejecuta `cargo run` — la tarea aparece en el menú automáticamente.

---

## Visualizaciones disponibles

| `VizKind`   | Descripción |
|-------------|-------------|
| `PlainText` | Muestra stdout como texto plano |
| `Stack`     | Parsea líneas `PUSH/POP \| STACK: …` y anima la pila |
| `Queue`     | Parsea líneas `ENQUEUE/DEQUEUE \| QUEUE: …` y anima la cola |

> **Trabajo Futuro**: se contemplan visualizaciones de árboles binarios, listas enlazadas, y gráficas (`ratatui::widgets::Chart`).

---

## ⚠️ Nota para usuarios de la red de Cinvestav

La red institucional de Cinvestav utiliza un sistema de filtrado (Fortiguard) que en ocasiones **bloquea el acceso a `crates.io`**, el registro de paquetes de Rust.

Si `cargo build` falla con un error como:
```
error: SSL peer certificate or SSH remote key was not OK
(SSL: certificate subject name 'Fortiguard SDNS Blocked Page'...)
```

Opciones para resolverlo:

1. **Hotspot de celular o VPN**: Conectarse a una red sin filtrado para la descarga inicial de dependencias (`cargo build`). Una vez descargadas, las compilaciones posteriores funcionan sin conexión.

2. **`cargo vendor`** (sin conexión):
   ```bash
   # En una máquina con internet libre:
   cargo vendor vendor/
   # Copia el directorio vendor/ al proyecto en Cinvestav
   # Agrega a Cargo.toml:
   # [source.crates-io]
   # replace-with = "vendored-sources"
   # [source.vendored-sources]
   # directory = "vendor"
   ```

3. **Reportar al equipo de IT**: El bloqueo es por el certificado SSL de Fortiguard, no por el contenido. Solicitar que `static.crates.io` y `crates.io` estén en la whitelist.

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