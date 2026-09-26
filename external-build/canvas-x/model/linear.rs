use crate::tensor::Int8Tensor;

pub struct Linear {
    pub weight: Int8Tensor,
}

impl Linear {
    /// Matrix-vector multiply: out[c] = Σ_r weight[r,c] * input[r]
    /// weight.dims = [in_dim, out_dim]; output length = out_dim.
    pub fn forward(&self, input: &[i8]) -> Vec<i8> {
        let rows = self.weight.dims[0];
        let cols = self.weight.dims[1];

        let mut out = vec![0i32; cols];

        let used_rows = rows.min(input.len());

        for r in 0..used_rows {
            for c in 0..cols {
                out[c] += (self.weight.data[r * cols + c] as i32) * (input[r] as i32);
            }
        }

        out.iter().map(|v| (v / 128) as i8).collect()
    }
}
