use sha2::{Digest, Sha256};

pub struct Lane {
    pub id: u8,
    pub payload: Vec<u8>,
}

pub fn parse_lanes(data: &[u8]) -> Vec<Lane> {
    let mut lanes = vec![];
    let mut i = 40; // skip 40-byte header

    while i < data.len() {
        let id = data[i];
        i += 1;

        let len = u32::from_be_bytes(data[i..i + 4].try_into().unwrap()) as usize;
        i += 4;

        let payload = data[i..i + len].to_vec();
        i += len;

        let stored_hash = &data[i..i + 32];
        i += 32;

        let computed = Sha256::digest(&payload);
        assert_eq!(
            computed.as_slice(),
            stored_hash,
            "lane {} hash mismatch — payload is corrupt",
            id
        );

        lanes.push(Lane { id, payload });
    }

    lanes
}
