use crate::tensor::Int8Tensor;

pub struct Embedding {
    pub weight: Int8Tensor,
}

impl Embedding {
    /// Look up a single token embedding row.
    pub fn forward(&self, token_id: usize) -> Vec<i8> {
        let dim = self.weight.dims[1];
        let start = token_id * dim;
        self.weight.data[start..start + dim].to_vec()
    }
}
