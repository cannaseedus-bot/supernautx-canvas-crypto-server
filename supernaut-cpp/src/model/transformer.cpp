// src/model/transformer.cpp
#include "model.h"

namespace supernaut {

S7Mini::S7Mini(size_t vocab_size, size_t hidden_dim)
    : embedding(vocab_size, hidden_dim),
      lm_head(hidden_dim, vocab_size),
      hidden_dim(hidden_dim),
      vocab_size(vocab_size) {}

FloatVec S7Mini::forward(const Int8Vec& token_ids) {
    if (token_ids.empty()) {
        return FloatVec(vocab_size, 0.0f);
    }
    
    // Get embedding for last token
    return forward_token(token_ids.back());
}

FloatVec S7Mini::forward_token(int8_t token_id) {
    // Get token embedding
    auto hidden = embedding.forward(token_id);
    
    // Project to logits
    return lm_head.forward(hidden);
}

} // namespace supernaut
