#pragma once

#include "tensor.h"
#include <unordered_map>

namespace supernaut {

/**
 * BPE Tokenizer: text <-> token IDs
 * 
 * Simple whitespace-split tokenizer with JSON vocabulary.
 * Maps words to token IDs (0-255) and back.
 */
class Tokenizer {
private:
    std::unordered_map<std::string, int8_t> vocab;      // word -> ID
    std::unordered_map<int8_t, std::string> inv_vocab;  // ID -> word
    int8_t unk_token = 0;  // Unknown token ID (default)
    
public:
    Tokenizer() = default;
    
    /**
     * Load vocabulary from JSON file
     * 
     * Expected format:
     * {
     *   "word1": "0",
     *   "word2": "1",
     *   ...
     * }
     * 
     * @param vocab_path Path to vocab.json file
     * @return Success (true if loaded, false on error)
     */
    bool load_vocab(const std::string& vocab_path);
    
    /**
     * Encode text to token IDs
     * 
     * Splits by whitespace, looks up each word in vocab.
     * Unknown words map to unk_token.
     * 
     * @param text Input text
     * @return Vector of token IDs
     */
    Int8Vec encode(const std::string& text);
    
    /**
     * Decode token IDs to text
     * 
     * Looks up each token ID, joins with spaces.
     * 
     * @param tokens Input token IDs
     * @return Reconstructed text
     */
    std::string decode(const Int8Vec& tokens);
    
    /**
     * Get vocabulary size
     */
    size_t size() const { return vocab.size(); }
    
    /**
     * Check if word in vocabulary
     */
    bool has_word(const std::string& word) const {
        return vocab.count(word) > 0;
    }
    
    /**
     * Get token ID for word (returns unk_token if not found)
     */
    int8_t get_token_id(const std::string& word) const {
        auto it = vocab.find(word);
        return it != vocab.end() ? it->second : unk_token;
    }
    
    /**
     * Set unknown token ID
     */
    void set_unk_token(int8_t token_id) { unk_token = token_id; }
};

} // namespace supernaut
