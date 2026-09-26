// src/model/attention.cpp
#include "model.h"
#include "supernaut_math.h"

namespace supernaut {

Attention::Attention(size_t hidden_dim)
    : q_proj(hidden_dim, hidden_dim),
      k_proj(hidden_dim, hidden_dim),
      v_proj(hidden_dim, hidden_dim),
      out_proj(hidden_dim, hidden_dim),
      hidden_dim(hidden_dim) {}

FloatVec Attention::forward(
    const FloatVec& input,
    const std::vector<FloatVec>& kv_cache)
{
    // Project input to Q, K, V
    auto queries = q_proj.forward(input);
    
    // Compute attention scores: scores[i] = queries · keys[i]
    std::vector<float> scores;
    for (const auto& k : kv_cache) {
        float score = 0.0f;
        for (size_t i = 0; i < queries.size(); ++i) {
            score += queries[i] * k[i];
        }
        scores.push_back(score);
    }
    
    // Softmax to get attention weights
    supernaut::math::softmax_inplace(scores.data(), scores.size());
    
    // Apply attention to values: output = weights · values
    std::vector<float> values;
    for (const auto& v : kv_cache) {
        values.push_back(0.0f);  // Simplified: would lookup actual values
    }
    
    FloatVec attn_output(hidden_dim, 0.0f);
    for (size_t i = 0; i < kv_cache.size(); ++i) {
        for (size_t j = 0; j < hidden_dim; ++j) {
            attn_output[j] += scores[i] * kv_cache[i][j];
        }
    }
    
    // Project output
    return out_proj.forward(attn_output);
}

} // namespace supernaut
