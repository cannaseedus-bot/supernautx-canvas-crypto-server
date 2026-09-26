// src/model/linear.cpp
#include "model.h"
#include "supernaut_math.h"

namespace supernaut {

Linear::Linear(size_t in_features, size_t out_features, float scale)
    : weight(in_features, out_features, scale), weight_scale(scale) {}

FloatVec Linear::forward(const FloatVec& input) {
    size_t out_size = weight.cols();
    FloatVec output(out_size, 0.0f);
    
    // Compute: output[j] = sum_i(input[i] * weight[i,j])
    for (size_t j = 0; j < out_size; ++j) {
        int32_t sum = 0;
        for (size_t i = 0; i < input.size(); ++i) {
            // Quantize input and multiply with INT8 weight
            int8_t input_q = static_cast<int8_t>(input[i] / weight_scale);
            sum += input_q * weight.get_raw(i, j);
        }
        // Dequantize: result * scale * scale
        output[j] = static_cast<float>(sum) * weight_scale * weight_scale;
    }
    
    return output;
}

} // namespace supernaut
