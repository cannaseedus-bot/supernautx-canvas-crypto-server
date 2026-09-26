#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace CanvasX {
namespace Distributed {

/// Multi-node MoE routing configuration
struct MoENode {
    std::string hostname;
    int port;
    std::string name;
    std::vector<int> expert_ids;
    
    std::string endpoint() const {
        return "http://" + hostname + ":" + std::to_string(port);
    }
};

/// Expert routing information
struct ExpertRoute {
    int expert_id;
    std::string node_name;
    std::string endpoint;
};

/// MoE cluster configuration
struct MoEClusterConfig {
    std::vector<MoENode> nodes;
    int total_experts;
    int top_k;  // Top-K routing
    bool fallback_to_moe;
    float latency_timeout_ms;
};

/// Query routing result
struct RoutingResult {
    std::vector<int> selected_experts;
    std::vector<float> routing_scores;
    std::vector<ExpertRoute> routes;
};

/// Multi-node MoE orchestrator
class DistributedMoEOrchestrator {
public:
    explicit DistributedMoEOrchestrator(const MoEClusterConfig& config);
    ~DistributedMoEOrchestrator() = default;

    /// Initialize cluster
    bool Initialize();

    /// Route query to experts
    RoutingResult RouteQuery(const std::vector<float>& logits);

    /// Execute on remote node
    std::vector<float> ExecuteRemote(
        const std::string& endpoint,
        int expert_id,
        const std::vector<float>& input
    );

    /// Combine expert outputs
    std::vector<float> AggregateOutputs(
        const std::vector<std::vector<float>>& expert_outputs,
        const std::vector<float>& routing_scores
    );

    /// Parallel dispatch to all selected experts
    std::vector<float> DispatchParallel(
        const std::vector<float>& input,
        const RoutingResult& routing
    );

    /// Health check node
    bool HealthCheck(const std::string& endpoint);

    /// Get cluster status
    std::string GetStatus() const;

    // Statistics
    struct Stats {
        uint64_t total_queries = 0;
        uint64_t successful_queries = 0;
        uint64_t failed_queries = 0;
        float avg_latency_ms = 0.0f;
        float avg_throughput_qps = 0.0f;
    };
    
    Stats GetStats() const { return stats_; }
    void ResetStats() { stats_ = Stats(); }

private:
    MoEClusterConfig config_;
    Stats stats_;
    
    std::vector<ExpertRoute> expert_routes_;
    
    void BuildRouteTable();
};

/// JSON RPC transport for distributed execution
class JsonRpcTransport {
public:
    /// Send RPC request
    static std::string SendRequest(
        const std::string& endpoint,
        const std::string& method,
        const std::map<std::string, std::string>& params
    );

    /// Parse RPC response
    static std::vector<float> ParseResponse(const std::string& response);

    /// Batch send to multiple endpoints
    static std::map<std::string, std::string> BatchSend(
        const std::vector<std::string>& endpoints,
        const std::string& method,
        const std::map<std::string, std::string>& params
    );
};

} // namespace Distributed
} // namespace CanvasX
