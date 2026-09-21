use ratatui::{
    layout::{Constraint, Direction, Layout, Rect},
    style::{Color, Modifier, Style},
    text::{Line, Span},
    widgets::{Block, Borders, Paragraph},
    Frame,
};

use super::VizFrame;

/// Parse the output of a stack C program into a sequence of animation frames.
///
/// Expected line format: `PUSH 40 | STACK: 10 20 30 40`
pub fn parse_output(output: &str) -> Vec<VizFrame> {
    let mut frames = vec![VizFrame {
        operation: "Estado inicial".into(),
        elements: vec![],
    }];
    for line in output.lines() {
        if let Some((op_part, state_part)) = line.split_once(" | STACK:") {
            let elements = state_part
                .split_whitespace()
                .filter_map(|s| s.parse::<i64>().ok())
                .collect();
            frames.push(VizFrame {
                operation: op_part.trim().to_string(),
                elements,
            });
        }
    }
    frames
}


/// Render a stack visualization.
///
/// The stack is drawn from bottom (oldest) to top (newest), with the top
/// element highlighted. Current operation is shown in a header bar.
///
/// Future enhancement: replace with `BarChart` widget for a bar-plot style.
pub fn render(frame: &mut Frame, area: Rect, frames: &[VizFrame], current: usize) {
    if frames.is_empty() {
        return;
    }
    let vf = &frames[current];

    // ── Layout: header (operation) + body (stack cells) ─────────────────────
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Length(3), Constraint::Min(0)])
        .split(area);

    // Header
    let header = Paragraph::new(Line::from(vec![
        Span::styled("Operación: ", Style::default().fg(Color::Yellow)),
        Span::styled(
            &vf.operation,
            Style::default()
                .fg(Color::Cyan)
                .add_modifier(Modifier::BOLD),
        ),
        Span::raw(format!(
            "   [{}/{}]  ← → para navegar",
            current + 1,
            frames.len()
        )),
    ]))
    .block(Block::default().borders(Borders::ALL).title(" Pila (LIFO) "));
    frame.render_widget(header, chunks[0]);

    // Body — stack cells stacked vertically, bottom→top
    let body_area = chunks[1];
    if vf.elements.is_empty() {
        let empty = Paragraph::new("(vacía)")
            .block(Block::default().borders(Borders::ALL).title(" Estado "));
        frame.render_widget(empty, body_area);
        return;
    }

    // One row per element; each row is 3 lines tall (border + value + border)
    let n = vf.elements.len();
    let row_h = 3u16;
    let total_h = n as u16 * row_h;

    // Determine vertical offset so the stack renders at the bottom of body_area
    let y_start = body_area
        .y
        .saturating_add(body_area.height.saturating_sub(total_h));

    // Draw from bottom of the stack (index 0) up to top (index n-1).
    // We render the last element at the top of the screen area.
    for (i, &val) in vf.elements.iter().enumerate() {
        let is_top = i == n - 1;
        let y = y_start + ((n - 1 - i) as u16) * row_h;
        if y + row_h > body_area.y + body_area.height {
            continue; // clamp to area
        }
        let cell_area = Rect {
            x: body_area.x,
            y,
            width: body_area.width,
            height: row_h,
        };
        let style = if is_top {
            Style::default()
                .fg(Color::Black)
                .bg(Color::Cyan)
                .add_modifier(Modifier::BOLD)
        } else {
            Style::default().fg(Color::White)
        };
        let label = if is_top {
            format!("  {val}  ◄ TOP")
        } else {
            format!("  {val}")
        };
        let cell = Paragraph::new(label)
            .style(style)
            .block(Block::default().borders(Borders::ALL));
        frame.render_widget(cell, cell_area);
    }
}
