use ratatui::{
    layout::{Constraint, Direction, Layout, Rect},
    style::{Color, Modifier, Style},
    text::{Line, Span},
    widgets::{Block, Borders, Paragraph},
    Frame,
};

use super::VizFrame;

/// Render a queue visualization.
///
/// Elements are displayed left-to-right with arrows between them.
/// FRONT is highlighted in green, REAR in yellow.
///
/// Future enhancement: wrap elements with scrolling when the queue is long,
/// or use a `Canvas` widget for an animated conveyor-belt effect.
pub fn render(frame: &mut Frame, area: Rect, frames: &[VizFrame], current: usize) {
    if frames.is_empty() {
        return;
    }
    let vf = &frames[current];

    // ── Layout: header + body ────────────────────────────────────────────────
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
    .block(Block::default().borders(Borders::ALL).title(" Cola circular (FIFO) "));
    frame.render_widget(header, chunks[0]);

    // Body
    let body_area = chunks[1];

    if vf.elements.is_empty() {
        let empty = Paragraph::new("(vacía)")
            .block(Block::default().borders(Borders::ALL).title(" Estado "));
        frame.render_widget(empty, body_area);
        return;
    }

    // Build a horizontal row of fixed-width cells separated by arrows.
    // Each cell is CELL_W columns wide.
    const CELL_W: u16 = 8;
    const ARROW_W: u16 = 3;
    let n = vf.elements.len() as u16;
    let total_w = n * CELL_W + (n.saturating_sub(1)) * ARROW_W;

    // Center horizontally
    let x_start = body_area
        .x
        .saturating_add(body_area.width.saturating_sub(total_w) / 2);
    // Center vertically (single row of 3 lines)
    let row_h = 5u16;
    let y_start = body_area
        .y
        .saturating_add(body_area.height.saturating_sub(row_h) / 2);

    for (i, &val) in vf.elements.iter().enumerate() {
        let is_front = i == 0;
        let is_rear = i == vf.elements.len() - 1;

        let x = x_start + i as u16 * (CELL_W + ARROW_W);

        // Draw arrow between elements
        if i > 0 {
            let arrow_area = Rect {
                x: x - ARROW_W,
                y: y_start + row_h / 2,
                width: ARROW_W,
                height: 1,
            };
            if arrow_area.x + arrow_area.width <= body_area.x + body_area.width {
                let arrow = Paragraph::new("→ ");
                frame.render_widget(arrow, arrow_area);
            }
        }

        // Draw cell
        let cell_area = Rect {
            x,
            y: y_start,
            width: CELL_W,
            height: row_h,
        };
        if cell_area.x + cell_area.width > body_area.x + body_area.width {
            break; // out of visible area
        }

        let (bg, label) = if is_front && is_rear {
            (Color::Magenta, format!(" {val}\nF+R"))
        } else if is_front {
            (Color::Green, format!(" {val}\nFRONT"))
        } else if is_rear {
            (Color::Yellow, format!(" {val}\nREAR"))
        } else {
            (Color::Reset, format!(" {val}"))
        };

        let style = Style::default()
            .fg(Color::Black)
            .bg(bg)
            .add_modifier(Modifier::BOLD);

        let cell = Paragraph::new(label)
            .style(if bg == Color::Reset {
                Style::default().fg(Color::White)
            } else {
                style
            })
            .block(Block::default().borders(Borders::ALL));
        frame.render_widget(cell, cell_area);
    }

    // Labels below
    let label_area = Rect {
        x: body_area.x,
        y: y_start + row_h + 1,
        width: body_area.width,
        height: 1,
    };
    let legend = Paragraph::new(Line::from(vec![
        Span::styled("■ FRONT ", Style::default().fg(Color::Green)),
        Span::styled("■ REAR ", Style::default().fg(Color::Yellow)),
        Span::styled("■ FRONT+REAR ", Style::default().fg(Color::Magenta)),
    ]));
    frame.render_widget(legend, label_area);
}
