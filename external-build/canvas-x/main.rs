mod s7l;
mod tensor;
mod model;
mod tokenizer;
mod inference;

use std::fs;
use inference::greedy::decode;
use tokenizer::bpe::Tokenizer;
use model::transformer::S7Mini;

fn main() {
    let model_bytes = fs::read("model/mini.s7l").expect("missing model");
    let s7 = s7l::S7File::parse(&model_bytes).expect("invalid s7l");

    let tokenizer = Tokenizer::from_file("model/vocab.json");
    let model = S7Mini::from_s7(&s7);

    let prompt = "hello world";
    let tokens = tokenizer.encode(prompt);

    let output = decode(&model, tokens, 32);

    println!("{}", tokenizer.decode(&output));
}
