# Documentación de la interfaz de terminal en Rust con Ratatui

Este documento describe la construcción de la interfaz de usuario de terminal (TUI) del proyecto **Programación Avanzada — Cinvestav CS 2026**, implementada en Rust utilizando las bibliotecas [`ratatui`](https://ratatui.rs/) y [`crossterm`](https://github.com/crossterm-rs/crossterm).

---

## Tabla de contenidos

1. [Diseño y estructura general](#1-diseño-y-estructura-general)
2. [Elementos de la interfaz](#2-elementos-de-la-interfaz)
3. [Controles de teclado](#3-controles-de-teclado)
4. [Visualización de estructuras de datos](#4-visualización-de-estructuras-de-datos)
5. [Manejo de errores](#5-manejo-de-errores)

---

## 1. Diseño y estructura general

La aplicación sigue el patrón **Elm Architecture** (Modelo–Vista–Actualización) adaptado a TUI:

```
main.rs
  └── tui::init()          ← inicializa la terminal
  └── App::run()           ← bucle principal (draw → handle_events)
        ├── ui::render()   ← despacha la vista según la pantalla activa
        └── handle_events()← lee teclas y muta el estado
```

### Módulos

| Módulo | Archivo | Responsabilidad |
|---|---|---|
| `tui` | [`tui.rs`](tui.rs) | Inicialización y restauración de la terminal |
| `app` | [`app.rs`](app.rs) | Estado global (`App`), bucle de eventos, transiciones de pantalla |
| `ui` | [`ui.rs`](ui.rs) | Renderizado de cada pantalla con Ratatui |
| `homework` | [`homework/mod.rs`](homework/mod.rs) | Registro de tareas, tipos `HomeworkEntry` y `VizKind` |
| `homework::runner` | [`homework/runner.rs`](homework/runner.rs) | Compilación con `gcc` y ejecución del binario |
| `viz` | [`viz/mod.rs`](viz/mod.rs) | Tipo `VizFrame` y parsers de salida de C |
| `viz::stack` | [`viz/stack.rs`](viz/stack.rs) | Widget de pila (LIFO) |
| `viz::queue` | [`viz/queue.rs`](viz/queue.rs) | Widget de cola circular (FIFO) |

### Ciclo de vida de la terminal

`tui::init()` habilita **raw mode** (crossterm), entra en la **pantalla alternativa** (`EnterAlternateScreen`) y construye un `Terminal<CrosstermBackend<Stdout>>`. Al salir (normal o por pánico), `tui::restore()` deshabilita raw mode y regresa a la pantalla principal (`LeaveAlternateScreen`), garantizando que la terminal del usuario quede limpia.

```
tui::init()
  ├── enable_raw_mode()
  ├── execute!(stdout, EnterAlternateScreen)
  └── Terminal::new(CrosstermBackend::new(stdout))

tui::restore()
  ├── disable_raw_mode()
  └── execute!(stdout, LeaveAlternateScreen)
```

### Estado de la aplicación (`App`)

```rust
pub struct App {
    pub screen: Screen,       // pantalla activa
    pub homeworks: Vec<HomeworkEntry>,
    pub selected: usize,      // índice resaltado en el menú
    pub should_quit: bool,
}
```

El enum `Screen` representa las cuatro pantallas posibles:

```rust
pub enum Screen {
    Menu,
    Output    { name: String, content: String },
    Visualization { name: String, frames: Vec<VizFrame>, current: usize },
    Error     { name: String, message: String },
}
```

---

## 2. Elementos de la interfaz

### 2.1 Pantalla de menú (`Screen::Menu`)

La pantalla principal se divide horizontalmente en dos paneles (55 % / 45 %):

```
┌─────────────────────────────────────┬──────────────────────────┐
│  Programación Avanzada — Cinvestav  │       Descripción        │
│                                     │                          │
│  ▶ Tarea 1 — ... (resaltado cyan)   │  Texto descriptivo de    │
│    Tarea 2 — ...                    │  la tarea seleccionada   │
│                                     │                          │
│   ↑↓/jk seleccionar  Enter ejecutar  q salir                   │
└─────────────────────────────────────┴──────────────────────────┘
```

- **Panel izquierdo** — `List` widget de Ratatui con `ListState`.  
  El ítem activo muestra el prefijo `▶` y color `Cyan` + `BOLD`. El fondo del ítem resaltado es `DarkGray`.
- **Panel derecho** — `Paragraph` con `Wrap` que muestra el campo `description` de la tarea seleccionada.
- **Barra de ayuda** — texto centrado en el borde inferior del panel izquierdo (`title_bottom`).

### 2.2 Pantalla de salida de texto (`Screen::Output`)

```
┌──────────────────────────────────────────────────────┐
│  Nombre de la tarea                                  │
│                                                      │
│  Salida estándar del programa C...                   │
│                                                      │
└──────────────────────────────────────────────────────┘
 Esc/q volver al menú
```

- Un `Paragraph` con `Wrap { trim: false }` ocupa toda la pantalla menos la última línea.
- La última línea muestra la ayuda en `DarkGray`.
- El layout vertical usa `Constraint::Min(0)` + `Constraint::Length(1)`.

### 2.3 Pantalla de visualización (`Screen::Visualization`)

Pantalla de paso a paso de una estructura de datos. Ocupa toda la terminal menos una línea de ayuda inferior. El contenido concreto depende del `VizKind` (ver §4).

### 2.4 Pantalla de error (`Screen::Error`)

```
┌──────────────────────────────────────────────────────┐  ← borde rojo
│  ✗ Error — Nombre de la tarea                        │
│                                                      │
│  Mensaje de error en rojo...                         │
│                                                      │
└──────────────────────────────────────────────────────┘
 Esc/q volver al menú
```

- `Paragraph` con `border_style` en `Color::Red` y texto en `Color::Red`.
- La ayuda se renderiza con `Rect` calculado manualmente en la última fila del área.

---

## 3. Controles de teclado

Todos los eventos se leen de forma bloqueante con `crossterm::event::read()`. Solo se procesan eventos `KeyEventKind::Press` para evitar duplicados en sistemas que emiten `Press + Release`.

### Menú principal

| Tecla | Acción |
|---|---|
| `↑` / `k` | Mover selección hacia arriba |
| `↓` / `j` | Mover selección hacia abajo |
| `Enter` | Compilar y ejecutar la tarea seleccionada |
| `q` / `Q` | Salir de la aplicación |

### Pantalla de salida / error

| Tecla | Acción |
|---|---|
| `↑` / `k` | Desplazar texto hacia arriba (1 línea) |
| `↓` / `j` | Desplazar texto hacia abajo (1 línea) |
| `PageUp` | Desplazar texto hacia arriba (10 líneas) |
| `PageDown` | Desplazar texto hacia abajo (10 líneas) |
| `Home` | Ir al inicio del texto |
| `Esc` / `q` / `Backspace` | Volver al menú principal |

### Pantalla de visualización

| Tecla | Acción |
|---|---|
| `→` / `l` | Avanzar al siguiente frame |
| `←` / `h` | Retroceder al frame anterior |
| `Home` | Ir al primer frame (estado inicial) |
| `End` | Ir al último frame |
| `Esc` / `q` / `Backspace` | Volver al menú principal |

---

## 4. Visualización de estructuras de datos

### 4.1 Modelo de datos: `VizFrame`

Cada paso de la animación se representa con:

```rust
pub struct VizFrame {
    pub operation: String,   // e.g. "PUSH 40", "DEQUEUE"
    pub elements: Vec<i64>,  // snapshot de la estructura en ese paso
}
```

El primer frame siempre es un estado inicial vacío (`operation: "Estado inicial"`, `elements: []`).

### 4.2 Parser de salida de C

Los programas en C deben imprimir una línea por operación con el formato:

```
OPERACION valor | ESTRUCTURA: e1 e2 e3 ...
```

Ejemplos:
```
PUSH 40 | STACK: 10 20 30 40
ENQUEUE 10 | QUEUE: 10
DEQUEUE | QUEUE: 20 30
```

`viz::parse_stack_output` y `viz::parse_queue_output` dividen cada línea por el separador ` | STACK:` / ` | QUEUE:` y parsean los enteros del lado derecho.

### 4.3 Visualización de pila — `viz::stack`

El área de visualización se divide verticalmente en **cabecera** (3 filas) + **cuerpo** (resto):

```
┌──────────────────────────────────────────────────────┐
│ Pila (LIFO)                                          │
│ Operación: PUSH 40   [4/6]  ← → para navegar        │
└──────────────────────────────────────────────────────┘
┌──────────────────────────────────────────────────────┐
│  40   ◄ TOP   (fondo cyan, texto negro, BOLD)        │
├──────────────────────────────────────────────────────┤
│  30             (blanco)                             │
├──────────────────────────────────────────────────────┤
│  20                                                  │
├──────────────────────────────────────────────────────┤
│  10                                                  │
└──────────────────────────────────────────────────────┘
```

- Cada elemento ocupa una celda de **3 filas** de alto (borde + valor + borde).
- Los elementos se dibujan de abajo hacia arriba; el de mayor índice (TOP) aparece en la parte superior.
- La pila se alinea al **fondo** del área de cuerpo: `y_start = body_area.height - total_height`.
- Si la pila está vacía se muestra `(vacía)`.

**Estilos:**

| Elemento | Fondo | Texto |
|---|---|---|
| TOP | `Cyan` | `Black` + `BOLD` |
| Resto | — | `White` |

### 4.4 Visualización de cola — `viz::queue`

```
┌──────────────────────────────────────────────────────┐
│ Cola circular (FIFO)                                 │
│ Operación: ENQUEUE 30   [3/5]  ← → para navegar     │
└──────────────────────────────────────────────────────┘

         ┌──────┐     ┌──────┐     ┌──────┐
         │  10  │  →  │  20  │  →  │  30  │
         │ FRONT│     │      │     │  REAR│
         └──────┘     └──────┘     └──────┘
         ■ FRONT  ■ REAR  ■ FRONT+REAR
```

- Las celdas miden `CELL_W = 8` columnas y `row_h = 5` filas; separadas por flechas `→` de `ARROW_W = 3` columnas.
- El bloque completo se **centra** horizontal y verticalmente dentro del área de cuerpo.
- Si una celda excede el ancho visible, se corta (`break`).
- Si la cola está vacía se muestra `(vacía)`.

**Estilos:**

| Posición | Fondo | Texto |
|---|---|---|
| FRONT | `Green` | `Black` + `BOLD` |
| REAR | `Yellow` | `Black` + `BOLD` |
| FRONT + REAR (único elemento) | `Magenta` | `Black` + `BOLD` |
| Interior | — (`Reset`) | `White` |

Una leyenda de colores se renderiza debajo de la fila de celdas.

---

## 5. Manejo de errores

### 5.1 Compilación y ejecución (`homework::runner`)

`compile_and_run` retorna `Result<String, String>`:

| Situación | Resultado |
|---|---|
| `gcc` no encontrado en `PATH` | `Err("No se pudo invocar gcc: ...")` |
| `gcc` retorna código ≠ 0 | `Err("Error de compilación (gcc):\n<stderr>")` |
| Binario retorna código ≠ 0 | `Err("El programa terminó con error:\n<stderr>\nSalida:\n<stdout>")` |
| Éxito | `Ok(<stdout>)` |

El binario se genera en la misma carpeta que el fuente, con extensión `.bin` (e.g. `c_src/Tarea1.bin`).

### 5.2 Pantalla de error en la TUI

Cuando `run_selected()` recibe un `Err`, la aplicación transiciona a `Screen::Error { name, message }`. El renderizador (`render_error`) muestra el mensaje completo con bordes y texto en **rojo**, permitiendo al usuario leer el error de compilación o de runtime directamente en la terminal sin abandonar la TUI.

### 5.3 Recuperación de la terminal ante pánicos

La función `main` en [`main.rs`](main.rs) llama a `tui::restore()` tras `App::run()` independientemente del resultado:

```rust
let result = app::App::new().run(&mut terminal);
tui::restore()?;
result
```

Así, incluso si ocurre un error de ejecución (`color_eyre::Result`), la terminal se restaura antes de propagar el error al shell.