// supernaut_orchestrator.hpp
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_DECLARE_NONSTDC_NAMES 1
#define _USE_MATH_DEFINES
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <ctime>
#include <algorithm>
#include "numatic_manifold_engine.hpp"
#include "daemon_session.hpp"
#include "numatic_deep_reasoner.hpp"

namespace Supernaut {

struct ExecutionResult {
    std::string specialist_id;
    std::string query;
    std::string response;
    double confidence;
    double latency_ms;
    bool success;
    time_t executed_at;
    std::string session_trace; // New: Trace link for UI
};

// NUMATIC MICRONAUT: Localized Semantic Manifold Operator
struct NumaticMicronaut {
    std::string id;
    std::string fold_authority;
    double entropy_budget;
    std::string description;
};

class SupernautOrchestrator {
public:
    SupernautOrchestrator();
    bool initialize();
    
    // Core Methods
    ExecutionResult invoke_agent(const std::string& agent_id, const std::string& thread_id, const std::string& input);
    ExecutionResult execute_query(const std::string& query, const std::string& thread_id = "default");
    
    // Local Model API
    ExecutionResult chat(const std::string& prompt);
    ExecutionResult generate(const std::string& prompt);
    ExecutionResult code(const std::string& prompt);
    
    // Discovery Methods
    std::string get_agents_json() const;
    std::string get_tools_json() const;
    std::string get_skills_json() const;
    std::string get_commands_json() const;
    
    // State Methods
    std::string create_thread();

private:
    std::map<std::string, NumaticMicronaut> micronauts_;
    std::map<std::string, DaemonSession> sessions_; // Replaces simple threads
    int total_queries_;
    time_t startup_time_;
    NumaticManifoldEngine numatic_engine_;
    NumaticDeepReasoner deep_reasoner_;

    bool load_micronaut(const std::string& path);
    DaemonSession& get_or_create_session(const std::string& id);
};

} // namespace Supernaut
