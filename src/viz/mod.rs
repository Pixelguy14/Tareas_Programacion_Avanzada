pub mod stack;
pub mod queue;

/// A single animation frame emitted by a data-structure program.
#[derive(Debug, Clone)]
pub struct VizFrame {
    /// Human-readable operation label, e.g. "PUSH 40".
    pub operation: String,
    /// Snapshot of the data structure contents at this step.
    pub elements: Vec<i64>,
}
