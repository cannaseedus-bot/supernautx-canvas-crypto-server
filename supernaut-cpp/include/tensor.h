#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace supernaut {

/**
 * Int8Tensor: Quantized tensor storage for INT8 operations
 * 
 * Stores data in INT8 format with a separate float scale factor.
 * All arithmetic uses the scale factor for dequantization.
 * 
 * Layout: Row-major (C-order)
 * Formula: value = int8_data[i] * scale
 */
struct Int8Tensor {
    std::vector<size_t> dims;    // Shape [rows, cols, ...]
    float scale;                 // Quantization scale factor
    std::vector<int8_t> data;    // Raw INT8 values

    /**
     * Constructor
     * @param rows First dimension
     * @param cols Second dimension
     * @param scale Quantization scale (default 1.0f = no scaling)
     */
    Int8Tensor(size_t rows, size_t cols, float scale = 1.0f)
        : dims{rows, cols}, scale(scale), 
          data(rows * cols, 0) {}

    /**
     * Access element (mutable)
     * @param r Row index
     * @param c Column index
     */
    int8_t& at(size_t r, size_t c) {
        return data[r * dims[1] + c];
    }

    /**
     * Access element (const)
     * @param r Row index
     * @param c Column index
     */
    int8_t at(size_t r, size_t c) const {
        return data[r * dims[1] + c];
    }

    /**
     * Get dequantized value (float)
     * @param r Row index
     * @param c Column index
     * @return Dequantized float value
     */
    float get_value(size_t r, size_t c) const {
        return static_cast<float>(at(r, c)) * scale;
    }

    /**
     * Get raw INT8 value
     * @param r Row index
     * @param c Column index
     * @return Raw INT8 value
     */
    int8_t get_raw(size_t r, size_t c) const {
        return at(r, c);
    }

    /**
     * Get dimensions
     */
    size_t rows() const { return dims[0]; }
    size_t cols() const { return dims.size() > 1 ? dims[1] : 1; }

    /**
     * Get total element count
     */
    size_t size() const { return data.size(); }

    /**
     * Clear tensor
     */
    void clear() {
        std::fill(data.begin(), data.end(), 0);
    }
};

// Forward declarations
struct Linear;
struct Attention;
struct FFN;
struct Embedding;
struct S7Mini;

// Type aliases
using FloatVec = std::vector<float>;
using Int8Vec = std::vector<int8_t>;

} // namespace supernaut
