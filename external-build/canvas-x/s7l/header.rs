pub struct Header {
    pub magic: [u8; 4],
    pub version: u16,
    pub class: u8,
    pub flags: u8,
    pub root_hash: [u8; 32],
}

impl Header {
    pub fn parse(data: &[u8]) -> Self {
        let mut root = [0u8; 32];
        root.copy_from_slice(&data[8..40]);

        Self {
            magic: data[0..4].try_into().unwrap(),
            version: u16::from_be_bytes(data[4..6].try_into().unwrap()),
            class: data[6],
            flags: data[7],
            root_hash: root,
        }
    }
}
