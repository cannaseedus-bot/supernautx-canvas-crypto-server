// llm_router.hpp
#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace Supernaut {

enum BackendProvider {
    OLLAMA_LOCAL,
    OLLAMA_CLOUD,
    OPENAI,
    ANTHROPIC,
    LLAMA_CPP,
    LM_STUDIO
};

struct LLMRequest {
    std::string prompt;
    std::string model;
    double temperature = 0.7;
    int max_tokens = 512;
    std::string endpoint_type; // chat, generation, code, message
};

class LLMRouter {
public:
    static std::string dispatch(const LLMRequest& req, BackendProvider provider) {
        std::cout << "[LLM-ROUTER] Dispatching " << req.endpoint_type 
                  << " to provider: " << get_provider_name(provider) << std::endl;
        
        // Polimorphic translation logic based on provider
        switch (provider) {
            case OLLAMA_LOCAL:
                return translate_to_ollama(req);
            case OPENAI:
                return translate_to_openai(req);
            case ANTHROPIC:
                return translate_to_anthropic(req);
            default:
                return "{\"error\": \"Provider translation not implemented\"}";
        }
    }

private:
    static std::string get_provider_name(BackendProvider p) {
        switch(p) {
            case OLLAMA_LOCAL: return "Ollama (Local)";
            case OPENAI: return "OpenAI";
            case ANTHROPIC: return "Anthropic";
            default: return "Other";
        }
    }

    static std::string translate_to_ollama(const LLMRequest& req) {
        return "{\"model\": \"" + req.model + "\", \"prompt\": \"" + req.prompt + "\", \"stream\": false}";
    }

    static std::string translate_to_openai(const LLMRequest& req) {
        return "{\"model\": \"" + req.model + "\", \"messages\": [{\"role\": \"user\", \"content\": \"" + req.prompt + "\"}]}";
    }

    static std::string translate_to_anthropic(const LLMRequest& req) {
        return "{\"model\": \"" + req.model + "\", \"max_tokens\": " + std::to_string(req.max_tokens) + ", \"messages\": [{\"role\": \"user\", \"content\": \"" + req.prompt + "\"}]}";
    }
};

} // namespace Supernaut
