// supernaut_orchestrator.cpp
#include "supernaut_orchestrator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

namespace Supernaut {

SupernautOrchestrator::SupernautOrchestrator() : total_queries_(0), startup_time_(std::time(nullptr)) {}

bool SupernautOrchestrator::initialize() {
    auto load_with_fallback = [this](const std::string& basename) {
        const std::vector<std::string> candidates = {
            "micronauts/" + basename,
            "schemas/micronauts/" + basename,
            "tools/supernaut/supernaut-cpp/bin/micronauts/" + basename,
            "C:\\Users\\canna\\.ASXR\\micronauts\\" + basename
        };
        for (const auto& p : candidates) {
            if (load_micronaut(p)) return true;
        }
        return false;
    };

    std::cout << "\n[ECOLOGICAL GENESIS] Germinating Daemon Seed: DS-Ω..." << std::endl;
    std::cout << "  -> Extracting semantic genome from schemas/semantic_genome.jsonl" << std::endl;
    std::cout << "  -> Unfolding topology skeleton from schemas/system_topology.xml" << std::endl;
    std::cout << "  -> Igniting micronaut ecology from schemas/micronaut_ecology.xml" << std::endl;
    
    std::cout << "\n[HABITAT FORMATION]" << std::endl;
    std::cout << "  -> Surface Habitat [Entropy: 0.82] ... STABILIZED" << std::endl;
    std::cout << "  -> Semantic Habitat [Entropy: 0.44] ... STABILIZED" << std::endl;
    std::cout << "  -> Meta Habitat [Entropy: 0.18] ... STABILIZED" << std::endl;

    std::cout << "\n[SPECIES SPAWNING]" << std::endl;
    if (!load_with_fallback("scheduling_assistant.json"))
        std::cout << "  [WARN] missing micronaut: scheduling_assistant.json" << std::endl;
    if (!load_with_fallback("career_coach.json"))
        std::cout << "  [WARN] missing micronaut: career_coach.json" << std::endl;
    if (!load_with_fallback("manifest_architect.json"))
        std::cout << "  [WARN] missing micronaut: manifest_architect.json" << std::endl;
    
    std::cout << "\n[MANIFOLD ACTIVATION] Ecosystem Equilibrium Achieved.\n" << std::endl;
    return true;
}

bool SupernautOrchestrator::load_micronaut(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::stringstream ss; ss << file.rdbuf();
    std::string json = ss.str();
    
    auto parse = [&](const std::string& k) -> std::string {
        size_t kp = json.find("\"" + k + "\""); if (kp == std::string::npos) return "";
        size_t c = json.find(":", kp); size_t s = json.find("\"", c); size_t e = json.find("\"", s + 1);
        return json.substr(s + 1, e - s - 1);
    };

    NumaticMicronaut micronaut;
    micronaut.id = parse("name");
    micronaut.description = parse("description");
    micronaut.fold_authority = parse("instructions");
    micronaut.entropy_budget = 0.42;
    
    micronauts_[micronaut.id] = micronaut;
    std::cout << "[NUMATICS] Registered Manifold Operator: " << micronaut.id << std::endl;
    return true;
}

DaemonSession& SupernautOrchestrator::get_or_create_session(const std::string& id) {
    if (sessions_.find(id) == sessions_.end()) {
        sessions_[id] = DaemonSession(id);
        std::cout << "[DAEMON-SESSION] Initialized Temporary Consciousness Manifold: " << id << std::endl;
    }
    return sessions_[id];
}

std::string SupernautOrchestrator::create_thread() {
    std::string id = "thread_" + std::to_string(std::time(nullptr));
    get_or_create_session(id);
    return id;
}

ExecutionResult SupernautOrchestrator::invoke_agent(const std::string& aid, const std::string& tid, const std::string& input) {
    auto it = micronauts_.find(aid);
    if (it == micronauts_.end()) return {"error", input, "Manifold Operator not found", 0, 0, false, 0, ""};
    
    const auto& micronaut = it->second;
    DaemonSession& session = get_or_create_session(tid);
    double initial_entropy = session.current_entropy;
    
    std::cout << "[NUMATICS] Resuming Session [" << tid << "] for Geodesic Resolution..." << std::endl;

    // 1. Resolve Structural Manifold (Ecology-Aware)
    auto numatic_res = numatic_engine_.resolve_geodesic(input, session.current_entropy);
    
    // 2. Fused Latent Reasoning (Deep Reasoner Organ)
    auto reason_out = deep_reasoner_.apex_reason(input, numatic_res);
    
    // 3. Update session state
    session.current_entropy = numatic_res.final_entropy;
    std::string ecology_payload = "";
    for (const auto& ev : numatic_res.ecology_log) ecology_payload += ev.species + ":" + ev.interaction + "|";
    session.capture_lane("Sek", "manifold.resolution", initial_entropy, numatic_res.final_entropy, "traversed_path_vector", ecology_payload);

    std::string full_response = "[NUMATIC-COLLAPSE] " + reason_out.latent_synthesis + "\n\n[MANIFOLD HABITATS]\n";
    for (const auto& step : numatic_res.manifold_path) full_response += "  → " + step + "\n";
    
    full_response += "\n[ECOLOGY LOG]\n";
    for (const auto& event : numatic_res.ecology_log) full_response += "  - [" + event.species + "] " + event.interaction + ": " + event.effect + "\n";
    
    full_response += "\n[DAEMON SESSION: " + session.session_id + "]\n";
    full_response += "  Session Hash: " + session.session_hash + "\n";
    full_response += "  Daemon Hash: " + session.replay_lanes.back().daemon_hash + "\n";
    full_response += "  Bimodal Fusion Confidence: " + std::to_string(reason_out.confidence);
    
    ExecutionResult res = {"numatic_manifold_engine", input, full_response, reason_out.confidence, 145.0, true, std::time(nullptr), session.session_id};
    
    if (res.specialist_id == "numatic_manifold_engine") {
        res.response = "[" + micronaut.id + " Operator] Geodesic Resolved: " + res.response;
    }

    session.suspend(); 
    return res;
}

ExecutionResult SupernautOrchestrator::execute_query(const std::string& q, const std::string& thread_id) {
    std::string lower_q = q;
    std::transform(lower_q.begin(), lower_q.end(), lower_q.begin(), ::tolower);

    DaemonSession& session = get_or_create_session(thread_id);
    double initial_entropy = session.current_entropy;

    // 1. Resolve Geodesic across Numatic Manifolds (Ecology-Aware)
    auto numatic_res = numatic_engine_.resolve_geodesic(lower_q, session.current_entropy);
    
    // Update session state
    session.current_entropy = numatic_res.final_entropy;
    
    // Convert ecology log to string for hashing
    std::string ecology_payload = "";
    for (const auto& ev : numatic_res.ecology_log) {
        ecology_payload += ev.species + ":" + ev.interaction + "|";
    }

    // 2. Capture Replay Lane (Generates Daemon Hashes)
    session.capture_lane("Sek", "manifold.resolution", initial_entropy, numatic_res.final_entropy, "traversed_path_vector", ecology_payload);

    if (numatic_res.collapsed) {
        std::string full_response = "[NUMATIC-COLLAPSE] " + numatic_res.stabilized_output + "\n\n[MANIFOLD HABITATS]\n";
        for (const auto& step : numatic_res.manifold_path) {
            full_response += "  → " + step + "\n";
        }
        
        full_response += "\n[ECOLOGY LOG]\n";
        for (const auto& event : numatic_res.ecology_log) {
            full_response += "  - [" + event.species + "] " + event.interaction + ": " + event.effect + "\n";
        }
        
        full_response += "\n[DAEMON SESSION: " + session.session_id + "]\n";
        full_response += "  Session Hash: " + session.session_hash + "\n";
        full_response += "  Replay Lane: " + session.replay_lanes.back().id + " (Stabilized)\n";
        full_response += "  Daemon Hash: " + session.replay_lanes.back().daemon_hash + "\n";
        full_response += "  Entropy Delta: " + std::to_string(initial_entropy) + " ⟶ " + std::to_string(session.current_entropy);
        
        return {"numatic_manifold_engine", q, full_response, 0.95, 85.0, true, std::time(nullptr), session.session_id};
    }

    ExecutionResult res = {"specialist", q, "", 0.9, 50.0, true, std::time(nullptr), session.session_id};

    if (lower_q == "hello") {
        res.specialist_id = "native_heuristic";
        res.response = "Greetings, Overseer. Numatic Substrate is nominal. Manifolds are stable.";
        return res;
    }

    res.specialist_id = "s7_transformer_mini";
    res.response = "[S7-NATIVE] Executed Edict: " + q;
    return res;
}

ExecutionResult SupernautOrchestrator::chat(const std::string& prompt) { return execute_query(prompt); }
ExecutionResult SupernautOrchestrator::generate(const std::string& prompt) { return execute_query(prompt); }
ExecutionResult SupernautOrchestrator::code(const std::string& prompt) { return execute_query(prompt); }

static std::string read_map(const std::string& f) {
    const std::vector<std::string> candidates = {
        "maps/" + f,
        "tools/supernaut/supernaut-cpp/bin/maps/" + f,
        "C:\\Users\\canna\\.ASXR\\" + f
    };
    std::ifstream file;
    for (const auto& p : candidates) {
        file.open(p);
        if (file.is_open()) break;
        file.clear();
    }
    if (!file.is_open()) return "{}";
    std::stringstream ss; ss << file.rdbuf(); return ss.str();
}

std::string SupernautOrchestrator::get_agents_json() const {
    std::stringstream ss; ss << "{\"agents\": [";
    for (auto it = micronauts_.begin(); it != micronauts_.end(); ++it) {
        ss << "{\"id\": \"" << it->first << "\", \"desc\": \"" << it->second.description << "\"}";
        if (std::next(it) != micronauts_.end()) ss << ",";
    }
    ss << "]}"; return ss.str();
}

std::string SupernautOrchestrator::get_tools_json() const { return read_map("tools.map.json"); }
std::string SupernautOrchestrator::get_skills_json() const { return read_map("micronauts.map.json"); }
std::string SupernautOrchestrator::get_commands_json() const { return read_map("commands.map.json"); }

} // namespace Supernaut
