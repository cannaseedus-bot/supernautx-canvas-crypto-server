use std::collections::HashMap;

use super::linear::Linear;
use crate::tensor::Int8Tensor;

const FIELD_LANE_ID: u8 = 0x02;
const EMBEDDING_NAME: &str = "embedding.weight";
const LM_HEAD_NAME: &str = "lm_head.weight";

pub struct S7Mini {
    pub embedding: Linear,
    pub lm_head: Linear,
}

impl S7Mini {
    /// Construct from a parsed .s7l file.
    /// FIELD lane (id=2) carries packed INT8 tensors with layout:
    /// [u16 name_len][name bytes][u8 rank][u32 dims...][f32 scale][i8 data...]
    pub fn from_s7(file: &crate::s7l::S7File) -> Self {
        if let Some(field_lane) = file.lane(FIELD_LANE_ID) {
            let tensors = parse_field_tensors(&field_lane.payload)
                .unwrap_or_else(|err| panic!("invalid FIELD lane tensor blob: {err}"));

            let embedding = tensors
                .get(EMBEDDING_NAME)
                .unwrap_or_else(|| panic!("missing tensor '{EMBEDDING_NAME}'"));
            let lm_head = tensors
                .get(LM_HEAD_NAME)
                .unwrap_or_else(|| panic!("missing tensor '{LM_HEAD_NAME}'"));

            validate_linear_tensor(EMBEDDING_NAME, embedding);
            validate_linear_tensor(LM_HEAD_NAME, lm_head);

            return Self {
                embedding: Linear {
                    weight: clone_tensor(embedding),
                },
                lm_head: Linear {
                    weight: clone_tensor(lm_head),
                },
            };
        }

        // Deterministic fallback for the minimal placeholder artifact (header-only mini.s7l).
        // Keeps the binary runnable until real FIELD tensors are sealed into the model.
        Self::bootstrap_default()
    }

    /// Forward pass: embed tokens → project to logits.
    pub fn forward(&self, tokens: &[i8]) -> Vec<i8> {
        let hidden = self.embedding.forward(tokens);
        self.lm_head.forward(&hidden)
    }

    fn bootstrap_default() -> Self {
        let embedding = Int8Tensor {
            // Supports up to 512-token context windows from the current decode loop.
            dims: vec![512, 320],
            scale: 1.0,
            data: vec![0; 512 * 320],
        };

        let lm_head = Int8Tensor {
            dims: vec![320, 256],
            scale: 1.0,
            data: vec![0; 320 * 256],
        };

        Self {
            embedding: Linear { weight: embedding },
            lm_head: Linear { weight: lm_head },
        }
    }
}

fn clone_tensor(t: &Int8Tensor) -> Int8Tensor {
    Int8Tensor {
        dims: t.dims.clone(),
        scale: t.scale,
        data: t.data.clone(),
    }
}

fn validate_linear_tensor(name: &str, t: &Int8Tensor) {
    assert_eq!(
        t.dims.len(),
        2,
        "tensor '{name}' must be rank-2 for Linear; got rank {}",
        t.dims.len()
    );
    let expected = t.numel();
    assert_eq!(
        t.data.len(),
        expected,
        "tensor '{name}' data length mismatch: {} != {}",
        t.data.len(),
        expected
    );
}

fn parse_field_tensors(payload: &[u8]) -> Result<HashMap<String, Int8Tensor>, String> {
    let mut tensors = HashMap::new();
    let mut i = 0usize;

    while i < payload.len() {
        if i + 2 > payload.len() {
            return Err("truncated name_len".into());
        }
        let name_len = u16::from_be_bytes([payload[i], payload[i + 1]]) as usize;
        i += 2;

        if i + name_len > payload.len() {
            return Err("truncated tensor name".into());
        }
        let name = std::str::from_utf8(&payload[i..i + name_len])
            .map_err(|_| "non-utf8 tensor name".to_string())?
            .to_string();
        i += name_len;

        if tensors.contains_key(&name) {
            return Err(format!("duplicate tensor name: {name}"));
        }

        if i + 1 > payload.len() {
            return Err(format!("truncated rank for tensor {name}"));
        }
        let rank = payload[i] as usize;
        i += 1;

        if rank == 0 {
            return Err(format!("tensor {name} has invalid rank 0"));
        }
        if i + rank * 4 > payload.len() {
            return Err(format!("truncated dims for tensor {name}"));
        }
        let mut dims = Vec::with_capacity(rank);
        for _ in 0..rank {
            let dim =
                u32::from_be_bytes([payload[i], payload[i + 1], payload[i + 2], payload[i + 3]])
                    as usize;
            i += 4;
            if dim == 0 {
                return Err(format!("tensor {name} has zero-sized dimension"));
            }
            dims.push(dim);
        }

        if i + 4 > payload.len() {
            return Err(format!("truncated scale for tensor {name}"));
        }
        let scale = f32::from_bits(u32::from_be_bytes([
            payload[i],
            payload[i + 1],
            payload[i + 2],
            payload[i + 3],
        ]));
        i += 4;

        let numel = dims.iter().product::<usize>();
        if i + numel > payload.len() {
            return Err(format!("truncated data for tensor {name}"));
        }
        let data = payload[i..i + numel].iter().map(|b| *b as i8).collect();
        i += numel;

        tensors.insert(name, Int8Tensor { dims, scale, data });
    }

    Ok(tensors)
}
