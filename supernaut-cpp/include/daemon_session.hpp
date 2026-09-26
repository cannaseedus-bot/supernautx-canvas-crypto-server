// daemon_session.hpp
// Native C++ Daemon Session Substrate
// Law: A Session is a temporary consciousness manifold.

#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include "daemon_hash.hpp"

namespace Supernaut {

struct ReplayLane {
    std::string id;
    std::string daemon_hash;
    std::string phase;
    std::string active_fold;
    double entropy_before;
    double entropy_after;
    std::vector<std::string> active_species;
    std::string geodesic_trace;
    time_t captured_at;
};

class DaemonSession {
public:
    std::string session_id;
    std::string session_hash;
    std::string manifold_region;
    double current_entropy;
    std::map<std::string, double> species_fitness;
    std::vector<ReplayLane> replay_lanes;
    time_t last_suspended;

    DaemonSession() : session_id("session_default"), manifold_region("root"), current_entropy(1.0), last_suspended(0) {
        update_hash();
    }
    
    DaemonSession(const std::string& id) : session_id(id), manifold_region("root"), current_entropy(1.0), last_suspended(0) {
        update_hash();
    }

    void update_hash() {
        session_hash = DaemonHash::hash_session(session_id, current_entropy);
    }

    void capture_lane(const std::string& phase, const std::string& fold, double ent_b, double ent_a, const std::string& geodesic, const std::string& ecology_log) {
        ReplayLane lane;
        lane.id = "RL-" + std::to_string(replay_lanes.size() + 1);
        lane.phase = phase;
        lane.active_fold = fold;
        lane.entropy_before = ent_b;
        lane.entropy_after = ent_a;
        lane.geodesic_trace = geodesic;
        lane.captured_at = std::time(nullptr);
        
        std::string lane_hash = DaemonHash::hash_lane(fold, geodesic, ent_b - ent_a);
        std::string eco_hash = DaemonHash::hash_ecology(ecology_log);
        lane.daemon_hash = DaemonHash::hash_daemon(session_hash, lane_hash, eco_hash);
        
        replay_lanes.push_back(lane);
        update_hash(); // Evolve session hash
    }

    void suspend() {
        last_suspended = std::time(nullptr);
        update_hash();
    }
    
    void resume() {
        // Restore manifold pressure states
        update_hash();
    }
};

} // namespace Supernaut

