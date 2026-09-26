/// http_client_native.hpp
/// Native WinHTTP-based HTTP client for agent dispatch
/// No external dependencies - uses Windows built-in WinHTTP API

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// ============================================================================
// HTTP REQUEST/RESPONSE TYPES
// ============================================================================

struct HttpResponse {
    int status_code = 0;
    std::string body = "";
    std::map<std::string, std::string> headers;
    std::string error_message = "";
    bool success = false;
};

struct HttpRequest {
    std::string method = "POST";     // GET, POST, etc.
    std::string url = "";
    std::string body = "";
    std::map<std::string, std::string> headers;
};

// ============================================================================
// NATIVE WINHTTP CLIENT
// ============================================================================

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    /// Execute HTTP request (blocking)
    HttpResponse Execute(const HttpRequest& request, int timeout_ms = 5000);
    
    /// Send and forget (returns immediately)
    bool ExecuteAsync(const HttpRequest& request);

private:
    HINTERNET session_handle = NULL;
    
    /// Parse URL into components
    struct UrlParts {
        std::wstring host;
        std::wstring path;
        int port = 80;
        bool secure = false;
    };
    
    UrlParts ParseUrl(const std::string& url);
    std::wstring StringToWide(const std::string& str);
    std::string WideToString(const std::wstring& str);
};

// ============================================================================
// AGENT DISPATCH CLIENT (Higher-level wrapper)
// ============================================================================

class AgentDispatcher {
public:
    /// Call an agent with input, get response
    std::string CallAgent(const std::string& agent_url, const std::string& input);
    
    /// Call multiple agents in sequence (pipeline)
    std::string CallAgentPipeline(
        const std::vector<std::string>& agent_urls,
        const std::string& initial_input
    );
    
private:
    HttpClient http_client;
    
    /// Build JSON request body for agent
    std::string BuildAgentRequest(const std::string& input);
    
    /// Parse JSON response from agent (simple string extraction)
    std::string ParseAgentResponse(const std::string& json_response);
};
