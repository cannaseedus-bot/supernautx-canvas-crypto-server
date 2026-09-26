use super::linear::Linear;

pub struct FFN {
    pub fc1: Linear,
    pub fc2: Linear,
}

/// Deterministic ReLU: max(0, x) for INT8.
fn relu(v: &[i8]) -> Vec<i8> {
    v.iter().map(|x| (*x).max(0)).collect()
}

impl FFN {
    pub fn forward(&self, hidden: &[i8]) -> Vec<i8> {
        let h = self.fc1.forward(hidden);
        let h = relu(&h);
        self.fc2.forward(&h)
    }
}
