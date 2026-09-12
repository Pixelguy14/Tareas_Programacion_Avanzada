mod app;
mod homework;
mod tui;
mod ui;
mod viz;

use color_eyre::Result;

fn main() -> Result<()> {
    color_eyre::install()?;
    let mut terminal = tui::init()?;
    let result = app::App::new().run(&mut terminal);
    tui::restore()?;
    result
}
