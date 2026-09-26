// src/inference/inference.cpp
#include "inference.h"
#include "tokenizer.h"

namespace supernaut {

// Inline argmax - finds index of maximum value
static size_t argmax_inline(const float* data, size_t size) {
    size_t max_idx = 0;
    float max_val = data[0];
    for (size_t i = 1; i < size; ++i) {
        if (data[i] > max_val) {
            max_val = data[i];
            max_idx = i;
        }
    }
    return max_idx;
}

Int8Vec greedy_decode(
    S7Mini& model,
    const Int8Vec& input_tokens,
    size_t max_tokens,
    int8_t eos_token)
{
    // Use raw array to avoid std::vector
    const size_t MAX_TOKENS = 512;
    int8_t token_buffer[MAX_TOKENS];
    size_t token_count = 0;
    
    // Copy input tokens
    for (size_t i = 0; i < input_tokens.size() && i < MAX_TOKENS; ++i) {
        token_buffer[i] = input_tokens[i];
        token_count++;
    }
    
    for (size_t step = 0; step < max_tokens && token_count < MAX_TOKENS; ++step) {
        // Get logits for next token (stub - would call model.forward in Phase 7.2)
        float logits[256] = {0};  // Placeholder
        
        // Greedy: select highest logit (argmax)
        size_t next_token_idx = argmax_inline(logits, 256);
        
        int8_t next_token = static_cast<int8_t>(next_token_idx);
        token_buffer[token_count++] = next_token;
        
        // Check for end-of-sequence
        if (eos_token != -1 && next_token == eos_token) {
            break;
        }
    }
    
    // Convert buffer back to vector
    Int8Vec result;
    for (size_t i = 0; i < token_count; ++i) {
        result.push_back(token_buffer[i]);
    }
    return result;
}

std::string generate_text(
    S7Mini& model,
    Tokenizer& tokenizer,
    const std::string& prompt,
    size_t max_tokens)
{
    // Encode prompt
    Int8Vec input_tokens = tokenizer.encode(prompt);
    
    // Generate tokens
    Int8Vec all_tokens = greedy_decode(model, input_tokens, max_tokens);
    
    // Extract generated tokens (skip prompt)
    Int8Vec generated;
    for (size_t i = input_tokens.size(); i < all_tokens.size(); ++i) {
        generated.push_back(all_tokens[i]);
    }
    
    // Decode generated tokens
    return tokenizer.decode(generated);
}

} // namespace supernaut
