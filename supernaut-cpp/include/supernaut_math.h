#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>

// Enable SIMD intrinsics based on platform
#ifdef _MSC_VER
    #include <intrin.h>
    #define SIMD_AVX2_AVAILABLE 1
#elif __GNUC__
    #include <immintrin.h>
    #define SIMD_AVX2_AVAILABLE 1
#else
    #define SIMD_AVX2_AVAILABLE 0
#endif

namespace supernaut {
namespace math {

/**
 * Check if AVX2 is available at runtime
 */
inline bool has_avx2() {
    #ifdef _MSC_VER
        int cpuinfo[4] = {};
        __cpuidex(cpuinfo, 7, 0);
        return (cpuinfo[1] & (1 << 5)) != 0;  // Bit 5 = AVX2
    #elif __GNUC__
        return __builtin_cpu_supports("avx2");
    #else
        return false;
    #endif
}

/**
 * INT8 dot product with AVX2 optimization
 * 
 * Computes: sum = a[0]*b[0] + a[1]*b[1] + ... + a[n-1]*b[n-1]
 * 
 * All inputs are INT8, output is INT32.
 * For use in quantized neural networks.
 * 
 * @param a First vector (INT8)
 * @param b Second vector (INT8)
 * @param len Vector length (must be > 0)
 * @return INT32 dot product result
 */
inline int32_t dot_product_int8(
    const int8_t* a,
    const int8_t* b,
    size_t len)
{
    // Use AVX2 version if available and length > 16
    #if SIMD_AVX2_AVAILABLE
    if (len > 16 && has_avx2()) {
        __m256i sum = _mm256_setzero_si256();
        
        size_t i = 0;
        for (; i + 32 <= len; i += 32) {
            // Load 32 INT8 values, sign-extend to INT32
            // Process in 2 batches of 16 to fit in __m256i
            
            // First 16 elements
            __m128i a_low = _mm_loadu_si128((__m128i*)(a + i));
            __m128i b_low = _mm_loadu_si128((__m128i*)(b + i));
            
            // Sign-extend INT8 → INT32
            __m256i a_vec = _mm256_cvtepi8_epi32(a_low);
            __m256i b_vec = _mm256_cvtepi8_epi32(b_low);
            
            // Multiply and add
            __m256i prod = _mm256_mullo_epi32(a_vec, b_vec);
            sum = _mm256_add_epi32(sum, prod);
            
            // Next 16 elements
            a_low = _mm_loadu_si128((__m128i*)(a + i + 16));
            b_low = _mm_loadu_si128((__m128i*)(b + i + 16));
            
            a_vec = _mm256_cvtepi8_epi32(a_low);
            b_vec = _mm256_cvtepi8_epi32(b_low);
            
            prod = _mm256_mullo_epi32(a_vec, b_vec);
            sum = _mm256_add_epi32(sum, prod);
        }
        
        // Horizontal sum
        __m256i tmp = _mm256_hadd_epi32(sum, _mm256_setzero_si256());
        tmp = _mm256_hadd_epi32(tmp, _mm256_setzero_si256());
        int32_t result = _mm256_extract_epi32(tmp, 0);
        
        // Handle remaining elements
        for (; i < len; ++i) {
            result += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
        }
        
        return result;
    }
    #endif
    
    // Scalar fallback
    int32_t result = 0;
    for (size_t i = 0; i < len; ++i) {
        result += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
    }
    return result;
}

/**
 * Quantize float vector to INT8
 * 
 * Converts float values to INT8 using a scale factor.
 * Clamps to [-128, 127] range.
 * 
 * @param src Source float vector
 * @param scale Quantization scale (output = input / scale)
 * @param len Vector length
 * @param dst Destination INT8 buffer
 */
inline void quantize_float_to_int8(
    const float* src,
    float scale,
    size_t len,
    int8_t* dst)
{
    for (size_t i = 0; i < len; ++i) {
        float val = src[i] / scale;
        // Clamp to INT8 range
        if (val > 127.0f) val = 127.0f;
        if (val < -128.0f) val = -128.0f;
        dst[i] = static_cast<int8_t>(std::round(val));
    }
}

/**
 * Dequantize INT8 vector to float
 * 
 * Converts INT8 values back to float using a scale factor.
 * 
 * @param src Source INT8 vector
 * @param scale Dequantization scale (output = input * scale)
 * @param len Vector length
 * @param dst Destination float buffer
 */
inline void dequantize_int8_to_float(
    const int8_t* src,
    float scale,
    size_t len,
    float* dst)
{
    for (size_t i = 0; i < len; ++i) {
        dst[i] = static_cast<float>(src[i]) * scale;
    }
}

/**
 * ReLU activation (in-place)
 * 
 * Applies ReLU: output[i] = max(0, input[i])
 * 
 * @param data Vector to transform (modified in-place)
 * @param len Vector length
 */
inline void relu_inplace(float* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (data[i] < 0.0f) data[i] = 0.0f;
    }
}

/**
 * Softmax normalization
 * 
 * Converts logits to probabilities: exp(x_i) / sum(exp(x))
 * Numerically stable using max subtraction.
 * 
 * @param logits Input logits (modified in-place)
 * @param len Vector length
 */
inline void softmax_inplace(float* logits, size_t len) {
    // Find max for numerical stability
    float max_val = logits[0];
    for (size_t i = 1; i < len; ++i) {
        if (logits[i] > max_val) max_val = logits[i];
    }
    
    // Compute exp(x - max) and sum
    float sum = 0.0f;
    for (size_t i = 0; i < len; ++i) {
        logits[i] = std::exp(logits[i] - max_val);
        sum += logits[i];
    }
    
    // Normalize
    float inv_sum = 1.0f / sum;
    for (size_t i = 0; i < len; ++i) {
        logits[i] *= inv_sum;
    }
}

/**
 * Find argmax (maximum value index)
 * 
 * @param values Array of values
 * @param len Array length
 * @return Index of maximum value
 */
inline size_t argmax(const float* values, size_t len) {
    size_t max_idx = 0;
    float max_val = values[0];
    
    for (size_t i = 1; i < len; ++i) {
        if (values[i] > max_val) {
            max_val = values[i];
            max_idx = i;
        }
    }
    
    return max_idx;
}

/**
 * Compute L2 norm of a vector
 * 
 * @param data Vector
 * @param len Vector length
 * @return L2 norm (sqrt of sum of squares)
 */
inline float l2_norm(const float* data, size_t len) {
    float sum_sq = 0.0f;
    for (size_t i = 0; i < len; ++i) {
        sum_sq += data[i] * data[i];
    }
    return std::sqrt(sum_sq);
}

}  // namespace math
}  // namespace supernaut
