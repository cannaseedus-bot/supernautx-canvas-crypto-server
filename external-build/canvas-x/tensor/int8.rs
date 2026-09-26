/// Quantized INT8 tensor. Weights are stored as i8 values.
/// Dequantized value = data[i] as f32 * scale (never used in proofs).
pub struct Int8Tensor {
    pub dims: Vec<usize>,
    pub scale: f32,
    pub data: Vec<i8>,
}

impl Int8Tensor {
    pub fn numel(&self) -> usize {
        self.dims.iter().product()
    }
}
