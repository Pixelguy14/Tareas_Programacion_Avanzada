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

/// Parse the output of `Test_Stack.c` into a sequence of frames.
/// Expected line format: `PUSH 40 | STACK: 10 20 30 40`
pub fn parse_stack_output(output: &str) -> Vec<VizFrame> {
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

/// Parse the output of `Test_Queue.c` into a sequence of frames.
/// Expected line format: `ENQUEUE 10 | QUEUE: 10`
pub fn parse_queue_output(output: &str) -> Vec<VizFrame> {
    let mut frames = vec![VizFrame {
        operation: "Estado inicial".into(),
        elements: vec![],
    }];
    for line in output.lines() {
        if let Some((op_part, state_part)) = line.split_once(" | QUEUE:") {
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
