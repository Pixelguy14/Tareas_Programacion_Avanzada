use ratatui::{
    layout::{Constraint, Direction, Layout, Rect},
    style::{Color, Modifier, Style},
    text::{Line, Span},
    widgets::{Block, Borders, List, ListItem, ListState, Paragraph, Wrap},
    Frame,
};

use crate::{
    app::{App, Screen},
    viz,
};

const TITLE: &str = " Programación Avanzada — Cinvestav CS 2026 ";
const HELP_MENU: &str = " ↑↓/jk seleccionar  Enter ejecutar  q salir ";
const HELP_OUTPUT: &str = " Esc/q volver al menú ";
const HELP_VIZ: &str = " ←→/hl paso a paso  Home inicio  End final  Esc/q volver ";

/// Main render dispatcher.
pub fn render(frame: &mut Frame, app: &App) {
    match &app.screen {
        Screen::Menu => render_menu(frame, app),
        Screen::Output { name, content } => render_output(frame, name, content),
        Screen::Error { name, message } => render_error(frame, name, message),
        Screen::Visualization { name, frames, current } => {
            render_viz(frame, name, frames, *current, app)
        }
    }
}

// ── Menu ─────────────────────────────────────────────────────────────────────

fn render_menu(frame: &mut Frame, app: &App) {
    let area = frame.area();

    // Split into left list + right description sidebar
    let chunks = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(55), Constraint::Percentage(45)])
        .split(area);

    // Left: homework list
    let items: Vec<ListItem> = app
        .homeworks
        .iter()
        .enumerate()
        .map(|(i, hw)| {
            let prefix = if i == app.selected { "▶ " } else { "  " };
            ListItem::new(Line::from(vec![
                Span::raw(prefix),
                Span::styled(
                    &hw.label,
                    if i == app.selected {
                        Style::default()
                            .fg(Color::Cyan)
                            .add_modifier(Modifier::BOLD)
                    } else {
                        Style::default().fg(Color::White)
                    },
                ),
            ]))
        })
        .collect();

    let mut list_state = ListState::default().with_selected(Some(app.selected));
    let list = List::new(items)
        .block(
            Block::default()
                .borders(Borders::ALL)
                .title(TITLE)
                .title_bottom(Line::from(HELP_MENU).centered()),
        )
        .highlight_style(Style::default().bg(Color::DarkGray));
    frame.render_stateful_widget(list, chunks[0], &mut list_state);

    // Right: description panel
    let desc = &app.homeworks[app.selected].description;
    let sidebar = Paragraph::new(desc.as_str())
        .block(
            Block::default()
                .borders(Borders::ALL)
                .title(" Descripción "),
        )
        .wrap(Wrap { trim: true });
    frame.render_widget(sidebar, chunks[1]);
}

// ── Plain text output ─────────────────────────────────────────────────────────

fn render_output(frame: &mut Frame, name: &str, content: &str) {
    let area = frame.area();
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(0), Constraint::Length(1)])
        .split(area);

    let p = Paragraph::new(content)
        .block(
            Block::default()
                .borders(Borders::ALL)
                .title(format!(" {name} ")),
        )
        .wrap(Wrap { trim: false });
    frame.render_widget(p, chunks[0]);

    let help = Paragraph::new(HELP_OUTPUT)
        .style(Style::default().fg(Color::DarkGray));
    frame.render_widget(help, chunks[1]);
}

// ── Error ─────────────────────────────────────────────────────────────────────

fn render_error(frame: &mut Frame, name: &str, message: &str) {
    let area = frame.area();
    let p = Paragraph::new(message)
        .block(
            Block::default()
                .borders(Borders::ALL)
                .title(format!(" ✗ Error — {name} "))
                .border_style(Style::default().fg(Color::Red)),
        )
        .wrap(Wrap { trim: false })
        .style(Style::default().fg(Color::Red));
    frame.render_widget(p, area);

    let help_area = Rect {
        x: area.x,
        y: area.y + area.height.saturating_sub(1),
        width: area.width,
        height: 1,
    };
    let help = Paragraph::new(HELP_OUTPUT).style(Style::default().fg(Color::DarkGray));
    frame.render_widget(help, help_area);
}

// ── Visualization ─────────────────────────────────────────────────────────────

fn render_viz(
    frame: &mut Frame,
    name: &str,
    frames: &[crate::viz::VizFrame],
    current: usize,
    app: &App,
) {
    let area = frame.area();
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(0), Constraint::Length(1)])
        .split(area);

    // Determine which visualizer to use based on the homework name
    let hw = &app.homeworks[app.selected];
    match hw.viz {
        crate::homework::VizKind::Stack => {
            viz::stack::render(frame, chunks[0], frames, current);
        }
        crate::homework::VizKind::Queue => {
            viz::queue::render(frame, chunks[0], frames, current);
        }
        _ => {
            // Fallback: shouldn't happen, but show raw frame data
            let content = frames
                .get(current)
                .map(|f| format!("{}: {:?}", f.operation, f.elements))
                .unwrap_or_default();
            let p = Paragraph::new(content)
                .block(Block::default().borders(Borders::ALL).title(format!(" {name} ")));
            frame.render_widget(p, chunks[0]);
        }
    }

    let help = Paragraph::new(HELP_VIZ).style(Style::default().fg(Color::DarkGray));
    frame.render_widget(help, chunks[1]);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Return a centered rectangle with given percentage size.
/// Kept for future modal popups (e.g., compiling spinner, help overlay).
#[allow(dead_code)]
fn centered_rect(percent_x: u16, percent_y: u16, r: Rect) -> Rect {
    let layout_v = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Percentage((100 - percent_y) / 2),
            Constraint::Percentage(percent_y),
            Constraint::Percentage((100 - percent_y) / 2),
        ])
        .split(r);

    Layout::default()
        .direction(Direction::Horizontal)
        .constraints([
            Constraint::Percentage((100 - percent_x) / 2),
            Constraint::Percentage(percent_x),
            Constraint::Percentage((100 - percent_x) / 2),
        ])
        .split(layout_v[1])[1]
}
