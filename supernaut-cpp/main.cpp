// main.cpp
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_DECLARE_NONSTDC_NAMES 1
#define _USE_MATH_DEFINES
#include "supernaut_orchestrator.hpp"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

using namespace Supernaut;
SupernautOrchestrator g_orch;

std::string json_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

std::string parse_json_string(const std::string& json, const std::string& key) {
    size_t k = json.find("\"" + key + "\""); if (k == std::string::npos) return "";
    size_t c = json.find(":", k); if (c == std::string::npos) return "";
    size_t s = json.find("\"", c); if (s == std::string::npos) return "";
    size_t e = json.find("\"", s + 1); if (e == std::string::npos) return "";
    return json.substr(s + 1, e - s - 1);
}

std::string handle_request(const std::string& method, const std::string& path, const std::string& body) {
    try {
        if (method == "OPTIONS") {
            return "{\"status\":\"cors_preflight_ok\"}";
        }
        
        if (path == "/health") return "{\"status\":\"ok\", \"substrate\":\"active\"}";
        if (path == "/agents") return g_orch.get_agents_json();
        if (path == "/tools") return g_orch.get_tools_json();
        if (path == "/skills") return g_orch.get_skills_json();
        if (path == "/commands") return g_orch.get_commands_json();
        if (path == "/thread/create") return "{\"thread_id\":\"" + g_orch.create_thread() + "\"}";
        
        if (method == "POST") {
            if (body.empty()) return "{\"error\":\"empty_payload\"}";
            
            if (path == "/agent/invoke") {
                std::string aid = parse_json_string(body, "agent_id");
                std::string tid = parse_json_string(body, "thread_id");
                std::string in = parse_json_string(body, "input");
                if (in.empty()) return "{\"error\":\"missing_input\"}";
                if (aid.empty()) aid = "SchedulingAssistant";
                auto res = g_orch.invoke_agent(aid, tid, in);
                return "{\"response\":\"" + json_escape(res.response) + "\", \"agent\":\"" + aid + "\", \"confidence\":" + std::to_string(res.confidence) + "}";
            }
            if (path == "/chat") {
                std::string p = parse_json_string(body, "prompt");
                if (p.empty()) return "{\"error\":\"missing_prompt\"}";
                auto res = g_orch.chat(p);
                return "{\"response\":\"" + json_escape(res.response) + "\", \"confidence\":" + std::to_string(res.confidence) + "}";
            }
            if (path == "/generation") {
                std::string p = parse_json_string(body, "prompt");
                auto res = g_orch.generate(p);
                return "{\"response\":\"" + json_escape(res.response) + "\", \"confidence\":" + std::to_string(res.confidence) + "}";
            }
            if (path == "/code") {
                std::string p = parse_json_string(body, "prompt");
                auto res = g_orch.code(p);
                return "{\"response\":\"" + json_escape(res.response) + "\", \"confidence\":" + std::to_string(res.confidence) + "}";
            }
            if (path == "/edict" || path == "/execute") {
                std::string q = parse_json_string(body, "query");
                if (q.empty()) q = parse_json_string(body, "edict");
                auto res = g_orch.execute_query(q);
                return "{\"response\": \"" + json_escape(res.response) + "\", \"confidence\":" + std::to_string(res.confidence) + "}";
            }
        }
        return "{\"error\":\"not_found\"}";
    } catch (const std::exception& e) {
        return "{\"error\":\"exception\", \"message\":\"" + std::string(e.what()) + "\"}";
    } catch (...) {
        return "{\"error\":\"unknown_crash\"}";
    }
}

int main() {
    std::cout << "Starting ASXR Phase 7.7 Native Substrate..." << std::endl;
    if (!g_orch.initialize()) { std::cerr << "Init failed" << std::endl; return 1; }

    WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
    struct addrinfo h, *r; ZeroMemory(&h, sizeof(h));
    h.ai_family = AF_INET; h.ai_socktype = SOCK_STREAM; h.ai_protocol = IPPROTO_TCP; h.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, "5776", &h, &r);
    SOCKET s = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    int opt = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    if (bind(s, r->ai_addr, (int)r->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        return 1;
    }
    listen(s, SOMAXCONN);
    std::cout << "ASXR Native Substrate online on port 5776" << std::endl;

    while (true) {
        SOCKET c = accept(s, NULL, NULL); if (c == INVALID_SOCKET) continue;
        char buffer[16384]; int n = recv(c, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0'; std::string req(buffer, n);
            size_t sep = req.find("\r\n\r\n");
            std::string body = (sep != std::string::npos) ? req.substr(sep + 4) : "";
            std::istringstream iss(req); std::string meth, path; iss >> meth >> path;
            
            std::cout << "[DEBUG] " << meth << " " << path << " (" << body.length() << " bytes)" << std::endl;
            
            std::string resp_body = handle_request(meth, path, body);
            std::stringstream hss;
            hss << "HTTP/1.1 200 OK\r\n";
            hss << "Content-Type: application/json\r\n";
            hss << "Content-Length: " << resp_body.length() << "\r\n";
            hss << "Access-Control-Allow-Origin: *\r\n";
            hss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
            hss << "Access-Control-Allow-Headers: Content-Type\r\n";
            hss << "Connection: close\r\n\r\n";
            hss << resp_body;
            std::string full = hss.str();
            send(c, full.c_str(), (int)full.length(), 0);
        }
        closesocket(c);
    }
    return 0;
}
