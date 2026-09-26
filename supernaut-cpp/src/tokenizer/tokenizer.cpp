// src/tokenizer/tokenizer.cpp
#include "tokenizer.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace supernaut {

// Simple JSON parser (nlohmann/json would be ideal, but this is minimal)
bool Tokenizer::load_vocab(const std::string& vocab_path) {
    std::ifstream file(vocab_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open vocab file: " << vocab_path << "\n";
        return false;
    }
    
    // Simple parsing: read lines, extract "word": "id" pairs
    // Format: {"word1": "0", "word2": "1", ...}
    std::string line;
    bool in_json = false;
    
    while (std::getline(file, line)) {
        // Skip braces and commas
        if (line.find('{') != std::string::npos) {
            in_json = true;
            continue;
        }
        if (!in_json || line.find('}') != std::string::npos) {
            break;
        }
        
        // Parse "word": "id" lines
        size_t quote1 = line.find('"');
        if (quote1 == std::string::npos) continue;
        
        size_t quote2 = line.find('"', quote1 + 1);
        if (quote2 == std::string::npos) continue;
        
        std::string word = line.substr(quote1 + 1, quote2 - quote1 - 1);
        
        size_t colon = line.find(':', quote2);
        size_t quote3 = line.find('"', colon);
        size_t quote4 = line.find('"', quote3 + 1);
        
        if (quote3 == std::string::npos || quote4 == std::string::npos) continue;
        
        std::string id_str = line.substr(quote3 + 1, quote4 - quote3 - 1);
        
        try {
            int id = std::stoi(id_str);
            if (id >= 0 && id <= 255) {
                int8_t token_id = static_cast<int8_t>(id);
                vocab[word] = token_id;
                inv_vocab[token_id] = word;
            }
        } catch (...) {
            continue;
        }
    }
    
    file.close();
    
    if (vocab.empty()) {
        std::cerr << "No vocabulary loaded from " << vocab_path << "\n";
        return false;
    }
    
    return true;
}

Int8Vec Tokenizer::encode(const std::string& text) {
    Int8Vec tokens;
    
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        auto it = vocab.find(word);
        if (it != vocab.end()) {
            tokens.push_back(it->second);
        } else {
            tokens.push_back(unk_token);
        }
    }
    
    return tokens;
}

std::string Tokenizer::decode(const Int8Vec& tokens) {
    std::string result;
    
    for (int8_t token : tokens) {
        auto it = inv_vocab.find(token);
        if (it != inv_vocab.end()) {
            if (!result.empty()) result += " ";
            result += it->second;
        }
    }
    
    return result;
}

} // namespace supernaut
