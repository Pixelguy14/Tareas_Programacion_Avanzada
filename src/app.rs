use color_eyre::Result;
use crossterm::event::{self, Event, KeyCode, KeyEventKind};

use crate::{
    homework::{registry, HomeworkEntry, VizKind},
    tui::Tui,
    ui,
    viz::{parse_queue_output, parse_stack_output, VizFrame},
};

/// Which screen the application is currently showing.
pub enum Screen {
    /// Main homework selection menu.
    Menu,
    /// Plain-text output of a compiled C program.
    Output {
        name: String,
        content: String,
    },
    /// Step-through visualization of a data structure.
    Visualization {
        name: String,
        frames: Vec<VizFrame>,
        current: usize,
    },
    /// Compilation / runtime error.
    Error {
        name: String,
        message: String,
    },
}

pub struct App {
    pub screen: Screen,
    pub homeworks: Vec<HomeworkEntry>,
    pub selected: usize,
    pub should_quit: bool,
}

impl App {
    pub fn new() -> Self {
        Self {
            screen: Screen::Menu,
            homeworks: registry(),
            selected: 0,
            should_quit: false,
        }
    }

    /// Main event loop — draws one frame then handles one key event.
    pub fn run(&mut self, terminal: &mut Tui) -> Result<()> {
        while !self.should_quit {
            terminal.draw(|frame| ui::render(frame, self))?;
            self.handle_events()?;
        }
        Ok(())
    }

    fn handle_events(&mut self) -> Result<()> {
        if let Event::Key(key) = event::read()? {
            if key.kind != KeyEventKind::Press {
                return Ok(());
            }
            match &self.screen {
                Screen::Menu => self.handle_menu(key.code),
                Screen::Output { .. } | Screen::Error { .. } => self.handle_output(key.code),
                Screen::Visualization { .. } => self.handle_viz(key.code),
            }
        }
        Ok(())
    }

    fn handle_menu(&mut self, code: KeyCode) {
        match code {
            KeyCode::Char('q') | KeyCode::Char('Q') => self.should_quit = true,
            KeyCode::Up | KeyCode::Char('k') => {
                if self.selected > 0 {
                    self.selected -= 1;
                }
            }
            KeyCode::Down | KeyCode::Char('j') => {
                if self.selected + 1 < self.homeworks.len() {
                    self.selected += 1;
                }
            }
            KeyCode::Enter => self.run_selected(),
            _ => {}
        }
    }

    fn handle_output(&mut self, code: KeyCode) {
        match code {
            KeyCode::Esc | KeyCode::Char('q') | KeyCode::Backspace => {
                self.screen = Screen::Menu;
            }
            _ => {}
        }
    }

    fn handle_viz(&mut self, code: KeyCode) {
        match code {
            KeyCode::Esc | KeyCode::Char('q') | KeyCode::Backspace => {
                self.screen = Screen::Menu;
            }
            KeyCode::Right | KeyCode::Char('l') => {
                if let Screen::Visualization { frames, current, .. } = &mut self.screen {
                    if *current + 1 < frames.len() {
                        *current += 1;
                    }
                }
            }
            KeyCode::Left | KeyCode::Char('h') => {
                if let Screen::Visualization { current, .. } = &mut self.screen {
                    if *current > 0 {
                        *current -= 1;
                    }
                }
            }
            KeyCode::Home => {
                if let Screen::Visualization { current, .. } = &mut self.screen {
                    *current = 0;
                }
            }
            KeyCode::End => {
                if let Screen::Visualization { frames, current, .. } = &mut self.screen {
                    *current = frames.len().saturating_sub(1);
                }
            }
            _ => {}
        }
    }

    /// Compile and run the selected homework, then transition to the
    /// appropriate screen depending on `VizKind`.
    fn run_selected(&mut self) {
        let hw = self.homeworks[self.selected].clone();
        let name = hw.label.clone();

        match crate::homework::runner::compile_and_run(&hw.src) {
            Err(msg) => {
                self.screen = Screen::Error { name, message: msg };
            }
            Ok(output) => match hw.viz {
                VizKind::PlainText => {
                    self.screen = Screen::Output {
                        name,
                        content: output,
                    };
                }
                VizKind::Stack => {
                    let frames = parse_stack_output(&output);
                    self.screen = Screen::Visualization {
                        name,
                        frames,
                        current: 0,
                    };
                }
                VizKind::Queue => {
                    let frames = parse_queue_output(&output);
                    self.screen = Screen::Visualization {
                        name,
                        frames,
                        current: 0,
                    };
                }
            },
        }
    }
}
