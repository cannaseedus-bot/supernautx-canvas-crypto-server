use sha2::{Digest, Sha256};

use super::lane::Lane;

/// Compute Merkle root over a slice of 32-byte hashes.
/// Pairs are hashed together; odd nodes are paired with themselves.
pub fn compute_merkle_root(hashes: &[[u8; 32]]) -> [u8; 32] {
    if hashes.is_empty() {
        return [0u8; 32];
    }
    if hashes.len() == 1 {
        return hashes[0];
    }

    let mut layer: Vec<[u8; 32]> = hashes.to_vec();
    while layer.len() > 1 {
        let mut next = vec![];
        let mut i = 0;
        while i < layer.len() {
            let left = layer[i];
            let right = if i + 1 < layer.len() { layer[i + 1] } else { layer[i] };
            let mut hasher = Sha256::new();
            hasher.update(left);
            hasher.update(right);
            let h: [u8; 32] = hasher.finalize().into();
            next.push(h);
            i += 2;
        }
        layer = next;
    }
    layer[0]
}

/// Verify that the Merkle root of all lane payloads matches the expected root.
pub fn verify_root(lanes: &[Lane], expected: &[u8; 32]) -> bool {
    let hashes: Vec<[u8; 32]> = lanes
        .iter()
        .map(|l| {
            let h: [u8; 32] = Sha256::digest(&l.payload).into();
            h
        })
        .collect();
    let root = compute_merkle_root(&hashes);
    root == *expected
}
