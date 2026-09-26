pub mod header;
pub mod lane;
pub mod merkle;

use header::Header;
use lane::{parse_lanes, Lane};

pub struct S7File {
    pub header: Header,
    pub lanes: Vec<Lane>,
}

impl S7File {
    pub fn parse(data: &[u8]) -> Result<Self, &'static str> {
        if data.len() < 40 {
            return Err("s7l too short — header requires 40 bytes");
        }
        if &data[0..4] != b"S7LM" {
            return Err("bad magic — expected S7LM");
        }
        let header = Header::parse(data);
        let lanes = parse_lanes(data);

        // Verify Merkle root over all lane payloads
        if !merkle::verify_root(&lanes, &header.root_hash) {
            return Err("Merkle root mismatch — sealed artifact is corrupt");
        }

        Ok(S7File { header, lanes })
    }

    pub fn lane(&self, id: u8) -> Option<&Lane> {
        self.lanes.iter().find(|l| l.id == id)
    }
}
