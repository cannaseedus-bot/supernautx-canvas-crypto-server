#pragma once

#include "tensor.h"
#include <memory>

namespace supernaut {

/**
 * Linear layer: y = x @ W + b
 * 
 * Matrix-vector multiplication with optional bias.
 * Uses INT8 quantized weights for efficiency.
 */
struct Linear {
    Int8Tensor weight;  // Shape: [in_features, out_features]
    float weight_scale;
    
    Linear() = default;
    
    /**
     * Constructor
     * @param in_features Input dimension
     * @param out_features Output dimension
     * @param scale Quantization scale factor
     */
    Linear(size_t in_features, size_t out_features, float scale = 1.0f);
    
    /**
     * Forward pass: y = x @ W
     * 
     * @param input Input vector [in_features]
     * @return Output vector [out_features]
     */
    FloatVec forward(const FloatVec& input);
    
    /**
     * Get output dimension
     */
    size_t out_features() const { return weight.cols(); }
    
    /**
     * Get input dimension
     */
    size_t in_features() const { return weight.rows(); }
};

/**
 * Attention layer: single-token multi-head attention
 * 
 * Computes: Attention(Q, K, V) = softmax(Q @ K^T / sqrt(d)) @ V
 * 
 * Simplified single-token version (no batching, no multi-head).
 */
struct Attention {
    Linear q_proj, k_proj, v_proj, out_proj;
    size_t hidden_dim;
    
    Attention() = default;
    
    /**
     * Constructor
     * @param hidden_dim Model dimension
     */
    Attention(size_t hidden_dim);
    
    /**
     * Forward pass with cached K, V
     * 
     * @param input Current token embedding [hidden_dim]
     * @param kv_cache Previous key/value cache [[seq_len, hidden_dim], [seq_len, hidden_dim]]
     * @return Attention output [hidden_dim]
     */
    FloatVec forward(
        const FloatVec& input,
        const std::vector<FloatVec>& kv_cache
    );
};

/**
 * Feed-forward network: x -> Dense(ReLU) -> Dense
 * 
 * Two-layer MLP with ReLU activation: y = Dense2(ReLU(Dense1(x)))
 */
struct FFN {
    Linear fc1;  // [hidden_dim, expansion_dim]
    Linear fc2;  // [expansion_dim, hidden_dim]
    
    FFN() = default;
    
    /**
     * Constructor
     * @param hidden_dim Model dimension
     * @param expansion_ratio Expansion factor (typically 4)
     */
    FFN(size_t hidden_dim, size_t expansion_ratio = 4);
    
    /**
     * Forward pass: Dense2(ReLU(Dense1(x)))
     * 
     * @param input Input vector [hidden_dim]
     * @return Output vector [hidden_dim]
     */
    FloatVec forward(const FloatVec& input);
};

/**
 * Embedding layer: token_id -> embedding_vector
 * 
 * Lookup table for converting token IDs to embeddings.
 */
struct Embedding {
    Int8Tensor weight;  // Shape: [vocab_size, embedding_dim]
    float weight_scale;
    
    Embedding() = default;
    
    /**
     * Constructor
     * @param vocab_size Number of tokens in vocabulary
     * @param embedding_dim Embedding dimension
     * @param scale Quantization scale
     */
    Embedding(size_t vocab_size, size_t embedding_dim, float scale = 1.0f);
    
    /**
     * Forward pass: lookup embedding for token
     * 
     * @param token_id Token ID (must be < vocab_size)
     * @return Embedding vector [embedding_dim]
     */
    FloatVec forward(int8_t token_id);
    
    /**
     * Get vocabulary size
     */
    size_t vocab_size() const { return weight.rows(); }
    
    /**
     * Get embedding dimension
     */
    size_t embedding_dim() const { return weight.cols(); }
};

/**
 * S7Mini: Minimal transformer language model
 * 
 * Architecture:
 *   - Embedding: tokens -> [hidden_dim]
 *   - LM Head: [hidden_dim] -> [vocab_size]
 * 
 * Simple 2-layer model for minimal inference.
 */
struct S7Mini {
    Embedding embedding;
    Linear lm_head;
    
    size_t hidden_dim;
    size_t vocab_size;
    
    S7Mini() = default;
    
    /**
     * Constructor
     * @param vocab_size Number of tokens
     * @param hidden_dim Model dimension
     */
    S7Mini(size_t vocab_size, size_t hidden_dim);
    
    /**
     * Forward pass: tokens -> logits
     * 
     * Gets embeddings for all tokens, uses last token to compute logits.
     * 
     * @param token_ids Input token sequence
     * @return Logits for next token [vocab_size]
     */
    FloatVec forward(const Int8Vec& token_ids);
    
    /**
     * Forward pass: single token embedding -> logits
     * 
     * @param token_id Single token ID
     * @return Logits [vocab_size]
     */
    FloatVec forward_token(int8_t token_id);
};

} // namespace supernaut
