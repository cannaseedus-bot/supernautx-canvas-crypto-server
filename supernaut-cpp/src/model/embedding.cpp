// src/model/embedding.cpp
#include "model.h"

namespace supernaut {

Embedding::Embedding(size_t vocab_size, size_t embedding_dim, float scale)
    : weight(vocab_size, embedding_dim, scale), weight_scale(scale) {}

FloatVec Embedding::forward(int8_t token_id) {
    std::vector<float> embedding(weight.cols());
    
    // Clamp token_id to valid range
    if (token_id < 0 || token_id >= static_cast<int8_t>(weight.rows())) {
        // Return zero vector for out-of-range tokens
        return embedding;
    }
    
    // Look up embedding row for token
    for (size_t i = 0; i < weight.cols(); ++i) {
        embedding[i] = weight.get_value(token_id, i);
    }
    
    return embedding;
}

} // namespace supernaut
