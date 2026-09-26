use super::linear::Linear;

pub struct Attention {
    pub q_proj: Linear,
    pub k_proj: Linear,
    pub v_proj: Linear,
    pub out_proj: Linear,
}

impl Attention {
    /// Single-token deterministic dot-product attention (no softmax needed for greedy).
    pub fn forward(&self, hidden: &[i8]) -> Vec<i8> {
        let q = self.q_proj.forward(hidden);
        let k = self.k_proj.forward(hidden);
        let v = self.v_proj.forward(hidden);

        // Dot-product score Q·K (integer arithmetic only)
        let score: i32 = q.iter().zip(k.iter()).map(|(a, b)| (*a as i32) * (*b as i32)).sum();
        let scale = (score >> 7) as i8;

        // Scale value vectors by attention score
        let attended: Vec<i8> = v
            .iter()
            .map(|x| ((*x as i32 * scale as i32) >> 7) as i8)
            .collect();

        self.out_proj.forward(&attended)
    }
}
