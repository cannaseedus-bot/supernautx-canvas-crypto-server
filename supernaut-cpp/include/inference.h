#pragma once

#include "model.h"

namespace supernaut {

class Tokenizer;

/**
 * Inference: token generation
 * 
 * Implements greedy decoding: repeatedly selecting highest-scoring token.
 */

/**
 * Generate tokens using greedy decoding
 * 
 * Repeatedly:
 * 1. Get logits from model
 * 2. Select token with highest logit (argmax)
 * 3. Append to token sequence
 * 4. Repeat until max_tokens or end-of-sequence
 * 
 * @param model S7Mini model
 * @param input_tokens Initial token sequence
 * @param max_tokens Maximum tokens to generate
 * @param eos_token End-of-sequence token ID (optional, -1 to disable)
 * @return Generated tokens (including input_tokens)
 */
Int8Vec greedy_decode(
    S7Mini& model,
    const Int8Vec& input_tokens,
    size_t max_tokens = 32,
    int8_t eos_token = 2
);

/**
 * Generate text from prompt
 * 
 * End-to-end: encode prompt -> generate tokens -> decode text
 * 
 * @param model S7Mini model
 * @param tokenizer Tokenizer
 * @param prompt Input text prompt
 * @param max_tokens Maximum tokens to generate
 * @return Generated text (without prompt)
 */
std::string generate_text(
    S7Mini& model,
    Tokenizer& tokenizer,
    const std::string& prompt,
    size_t max_tokens = 32
);

} // namespace supernaut
