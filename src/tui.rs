use std::io::{self, Stdout};

use crossterm::{
    execute,
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
};
use ratatui::{backend::CrosstermBackend, Terminal};

pub type Tui = Terminal<CrosstermBackend<Stdout>>;

/// Initialize the terminal in raw mode with an alternate screen.
pub fn init() -> io::Result<Tui> {
    enable_raw_mode()?;
    execute!(io::stdout(), EnterAlternateScreen)?;
    Terminal::new(CrosstermBackend::new(io::stdout()))
}

/// Restore the terminal to normal mode.
pub fn restore() -> io::Result<()> {
    disable_raw_mode()?;
    execute!(io::stdout(), LeaveAlternateScreen)
}
