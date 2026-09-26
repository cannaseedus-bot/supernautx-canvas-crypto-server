use crate::model::transformer::S7Mini;

/// Greedy decode: run `steps` forward passes, appending argmax token each time.
/// Deterministic — no sampling, no randomness.
pub fn decode(model: &S7Mini, mut tokens: Vec<i8>, steps: usize) -> Vec<i8> {
    for _ in 0..steps {
        let logits = model.forward(&tokens);
        let next = argmax(&logits);
        tokens.push(next as i8);
    }
    tokens
}

/// Return the index of the maximum value (greedy selection).
fn argmax(v: &[i8]) -> usize {
    v.iter()
        .enumerate()
        .max_by_key(|(_, val)| *val)
        .unwrap()
        .0
}
