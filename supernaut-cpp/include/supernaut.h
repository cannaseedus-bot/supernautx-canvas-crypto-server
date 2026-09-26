#pragma once

#include "model.h"
#include "tokenizer.h"
#include "inference.h"
#include <string>

/**
 * Supernaut: Main orchestration class
 * 
 * Coordinates model loading, tokenization, inference, and micronaut calls.
 */
class Supernaut {
private:
    S7Mini model;
    Tokenizer tokenizer;
    bool model_loaded = false;
    bool tokenizer_loaded = false;
    
public:
    Supernaut() = default;
    
    /**
     * Load model from S7L file
     * 
     * @param model_path Path to mini.s7l file
     * @return Success (true if loaded)
     */
    bool load_model(const std::string& model_path);
    
    /**
     * Load vocabulary from JSON file
     * 
     * @param vocab_path Path to vocab.json file
     * @return Success (true if loaded)
     */
    bool load_vocab(const std::string& vocab_path);
    
    /**
     * Generate text from prompt
     * 
     * @param prompt Input text
     * @param max_tokens Maximum tokens to generate
     * @return Generated text
     */
    std::string generate(const std::string& prompt, size_t max_tokens = 32);
    
    /**
     * Check if system is ready
     */
    bool is_ready() const { return model_loaded && tokenizer_loaded; }
    
    /**
     * Orchestrate request through micronaut network
     * 
     * Routes request to appropriate micronauts based on content.
     * Aggregates responses and generates final output.
     * 
     * @param request Input request text
     * @return Orchestrated response
     */
    std::string orchestrate(const std::string& request);
};
