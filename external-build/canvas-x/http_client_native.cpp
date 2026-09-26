/// http_client_native.cpp
/// WinHTTP implementation for native HTTP dispatch

#include "http_client_native.hpp"
#include <sstream>
#include <algorithm>
#include <thread>

// ============================================================================
// HTTP CLIENT IMPLEMENTATION
// ============================================================================

HttpClient::HttpClient() {
    // Create WinHTTP session
    session_handle = WinHttpOpen(
        L"Workspace/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
}

HttpClient::~HttpClient() {
    if (session_handle) {
        WinHttpCloseHandle(session_handle);
        session_handle = NULL;
    }
}

std::wstring HttpClient::StringToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
    return result;
}

std::string HttpClient::WideToString(const std::wstring& str) {
    if (str.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size, NULL, NULL);
    return result;
}

HttpClient::UrlParts HttpClient::ParseUrl(const std::string& url) {
    UrlParts parts;
    
    // Simple URL parser: "http://host:port/path" or "https://host:port/path"
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        parts.host = L"localhost";
        parts.path = L"/";
        return parts;
    }
    
    std::string scheme = url.substr(0, scheme_end);
    parts.secure = (scheme == "https");
    
    size_t host_start = scheme_end + 3;
    size_t path_start = url.find('/', host_start);
    
    if (path_start == std::string::npos) {
        parts.host = StringToWide(url.substr(host_start));
        parts.path = L"/";
    } else {
        std::string host_part = url.substr(host_start, path_start - host_start);
        parts.path = StringToWide(url.substr(path_start));
        
        // Parse host:port
        size_t port_sep = host_part.find(':');
        if (port_sep != std::string::npos) {
            parts.host = StringToWide(host_part.substr(0, port_sep));
            parts.port = std::stoi(host_part.substr(port_sep + 1));
        } else {
            parts.host = StringToWide(host_part);
            parts.port = parts.secure ? 443 : 80;
        }
    }
    
    return parts;
}

HttpResponse HttpClient::Execute(const HttpRequest& request, int timeout_ms) {
    HttpResponse response;
    
    if (!session_handle) {
        response.error_message = "HTTP session not initialized";
        return response;
    }
    
    // Parse URL
    UrlParts parts = ParseUrl(request.url);
    
    // Create connection
    HINTERNET connect_handle = WinHttpConnect(
        session_handle,
        parts.host.c_str(),
        parts.port,
        0
    );
    
    if (!connect_handle) {
        response.error_message = "Failed to connect";
        return response;
    }
    
    // Create request
    DWORD flags = WINHTTP_FLAG_ESCAPE_PERCENT;
    if (parts.secure) {
        flags |= WINHTTP_FLAG_SECURE;
    }
    
    HINTERNET request_handle = WinHttpOpenRequest(
        connect_handle,
        StringToWide(request.method).c_str(),
        parts.path.c_str(),
        L"HTTP/1.1",
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );
    
    if (!request_handle) {
        response.error_message = "Failed to create request";
        WinHttpCloseHandle(connect_handle);
        return response;
    }
    
    // Set timeout
    WinHttpSetTimeouts(request_handle, timeout_ms, timeout_ms, timeout_ms, timeout_ms);
    
    // Add headers
    for (const auto& header : request.headers) {
        std::string header_line = header.first + ": " + header.second;
        WinHttpAddRequestHeaders(
            request_handle,
            StringToWide(header_line).c_str(),
            (ULONG)-1L,
            WINHTTP_ADDREQ_FLAG_ADD
        );
    }
    
    // Add Content-Type if body present
    if (!request.body.empty()) {
        WinHttpAddRequestHeaders(
            request_handle,
            L"Content-Type: application/json",
            (ULONG)-1L,
            WINHTTP_ADDREQ_FLAG_ADD
        );
    }
    
    // Send request
    BOOL send_result = WinHttpSendRequest(
        request_handle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)request.body.c_str(),
        (DWORD)request.body.size(),
        (DWORD)request.body.size(),
        0
    );
    
    if (!send_result) {
        response.error_message = "Failed to send request";
        WinHttpCloseHandle(request_handle);
        WinHttpCloseHandle(connect_handle);
        return response;
    }
    
    // Receive response
    if (!WinHttpReceiveResponse(request_handle, NULL)) {
        response.error_message = "Failed to receive response";
        WinHttpCloseHandle(request_handle);
        WinHttpCloseHandle(connect_handle);
        return response;
    }
    
    // Get status code
    DWORD status_code = 0;
    DWORD size = sizeof(status_code);
    WinHttpQueryHeaders(
        request_handle,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status_code,
        &size,
        WINHTTP_NO_HEADER_INDEX
    );
    response.status_code = (int)status_code;
    
    // Read response body
    DWORD bytes_available = 0;
    while (WinHttpQueryDataAvailable(request_handle, &bytes_available) && bytes_available > 0) {
        std::vector<char> buffer(bytes_available + 1, 0);
        
        DWORD bytes_read = 0;
        if (WinHttpReadData(request_handle, (LPVOID)&buffer[0], bytes_available, &bytes_read)) {
            response.body.append(&buffer[0], bytes_read);
        } else {
            break;
        }
    }
    
    response.success = (response.status_code >= 200 && response.status_code < 300);
    
    // Cleanup
    WinHttpCloseHandle(request_handle);
    WinHttpCloseHandle(connect_handle);
    
    return response;
}

bool HttpClient::ExecuteAsync(const HttpRequest& request) {
    // For MVP, just execute synchronously in a thread
    // Full implementation would use thread pool + completion callbacks
    std::thread([this, request]() {
        Execute(request, 5000);
    }).detach();
    return true;
}

// ============================================================================
// AGENT DISPATCHER IMPLEMENTATION
// ============================================================================

std::string AgentDispatcher::BuildAgentRequest(const std::string& input) {
    // Simple JSON-like request: {"input": "..."}
    std::string escaped = input;
    // Escape quotes in input
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    return "{\"input\": \"" + escaped + "\"}";
}

std::string AgentDispatcher::ParseAgentResponse(const std::string& json_response) {
    // Simple extraction: look for "output" or "result" field
    size_t result_pos = json_response.find("\"output\"");
    if (result_pos == std::string::npos) {
        result_pos = json_response.find("\"result\"");
    }
    if (result_pos == std::string::npos) {
        result_pos = json_response.find("\"response\"");
    }
    
    if (result_pos == std::string::npos) {
        return json_response;  // Return raw response if no field found
    }
    
    // Find the value after the colon
    size_t colon_pos = json_response.find(':', result_pos);
    if (colon_pos == std::string::npos) {
        return json_response;
    }
    
    // Extract string between quotes
    size_t quote_start = json_response.find('"', colon_pos);
    if (quote_start == std::string::npos) {
        return json_response;
    }
    
    size_t quote_end = json_response.find('"', quote_start + 1);
    if (quote_end == std::string::npos) {
        return json_response;
    }
    
    return json_response.substr(quote_start + 1, quote_end - quote_start - 1);
}

std::string AgentDispatcher::CallAgent(const std::string& agent_url, const std::string& input) {
    HttpRequest request;
    request.method = "POST";
    request.url = agent_url;
    request.body = BuildAgentRequest(input);
    request.headers["Accept"] = "application/json";
    
    HttpResponse response = http_client.Execute(request, 5000);
    
    if (!response.success) {
        return "ERROR: " + response.error_message + " (HTTP " + std::to_string(response.status_code) + ")";
    }
    
    return ParseAgentResponse(response.body);
}

std::string AgentDispatcher::CallAgentPipeline(
    const std::vector<std::string>& agent_urls,
    const std::string& initial_input
) {
    std::string result = initial_input;
    
    for (const auto& url : agent_urls) {
        result = CallAgent(url, result);
        
        // If error occurred, stop pipeline
        if (result.find("ERROR:") == 0) {
            break;
        }
    }
    
    return result;
}

