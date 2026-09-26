use std::collections::HashMap;
use std::fs;

use serde_json::Value;

pub struct Tokenizer {
    vocab: HashMap<String, i8>,
    inv_vocab: HashMap<i8, String>,
}

impl Tokenizer {
    pub fn from_file(path: &str) -> Self {
        let data = fs::read_to_string(path).expect("missing vocab.json");
        let v: Value = serde_json::from_str(&data).expect("invalid vocab JSON");
        let obj = v.as_object().expect("vocab must be a JSON object");

        let mut vocab = HashMap::new();
        let mut inv_vocab = HashMap::new();

        for (token, id_val) in obj {
            let id = id_val.as_i64().expect("token id must be integer") as i8;
            vocab.insert(token.clone(), id);
            inv_vocab.insert(id, token.clone());
        }

        Self { vocab, inv_vocab }
    }

    /// Encode whitespace-split words to token IDs.
    /// Unknown words map to 0 (<unk>).
    pub fn encode(&self, text: &str) -> Vec<i8> {
        text.split_whitespace()
            .map(|w| *self.vocab.get(w).unwrap_or(&0))
            .collect()
    }

    /// Decode token IDs back to space-separated words.
    pub fn decode(&self, tokens: &[i8]) -> String {
        tokens
            .iter()
            .map(|id| self.inv_vocab.get(id).cloned().unwrap_or_else(|| "<unk>".to_string()))
            .collect::<Vec<_>>()
            .join(" ")
    }
}
