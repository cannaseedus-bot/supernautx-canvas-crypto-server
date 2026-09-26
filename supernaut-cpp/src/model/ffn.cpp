// src/model/ffn.cpp
#include "model.h"
#include "supernaut_math.h"

namespace supernaut {

FFN::FFN(size_t hidden_dim, size_t expansion_ratio)
    : fc1(hidden_dim, hidden_dim * expansion_ratio),
      fc2(hidden_dim * expansion_ratio, hidden_dim) {}

FloatVec FFN::forward(const FloatVec& input) {
    // First layer with ReLU: fc1(input)
    auto hidden = fc1.forward(input);
    
    // ReLU activation
    supernaut::math::relu_inplace(hidden.data(), hidden.size());
    
    // Second layer: fc2(hidden)
    return fc2.forward(hidden);
}

} // namespace supernaut
