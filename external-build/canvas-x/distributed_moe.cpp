#include "distributed_moe.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <thread>
#include <future>

namespace CanvasX {
namespace Distributed {

DistributedMoEOrchestrator::DistributedMoEOrchestrator(const MoEClusterConfig& config)
    : config_(config) {
    BuildRouteTable();
}

bool DistributedMoEOrchestrator::Initialize() {
    std::cout << "Initializing " << config_.nodes.size() << " nodes...\n";
    
    for (const auto& node : config_.nodes) {
        if (HealthCheck(node.endpoint())) {
            std::cout << "✓ " << node.name << " (" << node.endpoint() << ") online\n";
        } else {
            std::cout << "✗ " << node.name << " OFFLINE\n";
            return false;
        }
    }
    
    std::cout << "Cluster initialized with " << config_.total_experts << " experts\n";
    return true;
}

void DistributedMoEOrchestrator::BuildRouteTable() {
    expert_routes_.clear();
    
    for (const auto& node : config_.nodes) {
        for (int expert_id : node.expert_ids) {
            expert_routes_.push_back({
                expert_id,
                node.name,
                node.endpoint()
            });
        }
    }
    
    std::cout << "Built route table for " << expert_routes_.size() << " experts\n";
}

RoutingResult DistributedMoEOrchestrator::RouteQuery(const std::vector<float>& logits) {
    RoutingResult result;
    
    if (logits.size() != config_.total_experts) {
        std::cerr << "ERROR: Logits size " << logits.size() 
                  << " != expected " << config_.total_experts << "\n";
        return result;
    }
    
    // Top-K selection with scores
    std::vector<std::pair<float, int>> scores;
    for (size_t i = 0; i < logits.size(); i++) {
        scores.push_back({logits[i], (int)i});
    }
    
    // Sort by score descending
    std::sort(scores.begin(), scores.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Select top-K
    int k = std::min(config_.top_k, (int)scores.size());
    float score_sum = 0.0f;
    
    for (int i = 0; i < k; i++) {
        result.selected_experts.push_back(scores[i].second);
        result.routing_scores.push_back(scores[i].first);
        score_sum += scores[i].first;
    }
    
    // Normalize scores
    if (score_sum > 0) {
        for (auto& score : result.routing_scores) {
            score /= score_sum;
        }
    }
    
    // Find node endpoints for each expert
    for (int expert_id : result.selected_experts) {
        auto it = std::find_if(expert_routes_.begin(), expert_routes_.end(),
                              [expert_id](const ExpertRoute& r) {
                                  return r.expert_id == expert_id;
                              });
        if (it != expert_routes_.end()) {
            result.routes.push_back(*it);
        }
    }
    
    return result;
}

std::vector<float> DistributedMoEOrchestrator::ExecuteRemote(
    const std::string& endpoint,
    int expert_id,
    const std::vector<float>& input) {
    
    // JSON RPC request
    std::map<std::string, std::string> params;
    params["expert_id"] = std::to_string(expert_id);
    params["input_size"] = std::to_string(input.size());
    
    // In real implementation, would serialize input vector to JSON
    std::string response = JsonRpcTransport::SendRequest(
        endpoint,
        "execute_expert",
        params
    );
    
    return JsonRpcTransport::ParseResponse(response);
}

std::vector<float> DistributedMoEOrchestrator::AggregateOutputs(
    const std::vector<std::vector<float>>& expert_outputs,
    const std::vector<float>& routing_scores) {
    
    if (expert_outputs.empty()) {
        return {};
    }
    
    size_t output_dim = expert_outputs[0].size();
    std::vector<float> aggregated(output_dim, 0.0f);
    
    for (size_t i = 0; i < expert_outputs.size(); i++) {
        if (expert_outputs[i].size() != output_dim) {
            std::cerr << "ERROR: Output dimension mismatch\n";
            continue;
        }
        
        float score = (i < routing_scores.size()) ? routing_scores[i] : 0.0f;
        
        for (size_t j = 0; j < output_dim; j++) {
            aggregated[j] += expert_outputs[i][j] * score;
        }
    }
    
    return aggregated;
}

std::vector<float> DistributedMoEOrchestrator::DispatchParallel(
    const std::vector<float>& input,
    const RoutingResult& routing) {
    
    std::vector<std::future<std::vector<float>>> futures;
    
    // Launch parallel tasks
    for (size_t i = 0; i < routing.routes.size(); i++) {
        const auto& route = routing.routes[i];
        
        auto task = std::async(std::launch::async,
            [this, &input, &route]() {
                return ExecuteRemote(route.endpoint, route.expert_id, input);
            });
        
        futures.push_back(std::move(task));
    }
    
    // Collect results
    std::vector<std::vector<float>> outputs;
    for (auto& future : futures) {
        try {
            outputs.push_back(future.get());
        } catch (const std::exception& e) {
            std::cerr << "Expert execution failed: " << e.what() << "\n";
            stats_.failed_queries++;
        }
    }
    
    // Aggregate
    return AggregateOutputs(outputs, routing.routing_scores);
}

bool DistributedMoEOrchestrator::HealthCheck(const std::string& endpoint) {
    try {
        std::string response = JsonRpcTransport::SendRequest(
            endpoint,
            "health",
            {}
        );
        return !response.empty();
    } catch (const std::exception& e) {
        std::cerr << "Health check failed: " << e.what() << "\n";
        return false;
    }
}

std::string DistributedMoEOrchestrator::GetStatus() const {
    std::string status = "=== Distributed MoE Cluster Status ===\n";
    status += "Nodes: " + std::to_string(config_.nodes.size()) + "\n";
    status += "Total experts: " + std::to_string(config_.total_experts) + "\n";
    status += "Top-K: " + std::to_string(config_.top_k) + "\n";
    status += "Queries: " + std::to_string(stats_.total_queries) + "\n";
    status += "Success rate: " + 
        std::to_string((stats_.total_queries > 0) ? 
            (100.0 * stats_.successful_queries / stats_.total_queries) : 0.0) + "%\n";
    status += "Avg latency: " + std::to_string(stats_.avg_latency_ms) + " ms\n";
    status += "Throughput: " + std::to_string(stats_.avg_throughput_qps) + " q/s\n";
    
    return status;
}

// JSON RPC Transport Implementation
std::string JsonRpcTransport::SendRequest(
    const std::string& endpoint,
    const std::string& method,
    const std::map<std::string, std::string>& params) {
    
    // Simplified: in real implementation would use curl/http client
    // For now, return mock response
    
    if (method == "health") {
        return R"({"status":"ok"})";
    } else if (method == "execute_expert") {
        return R"({"result":[1.0,2.0,3.0]})";
    }
    
    return "";
}

std::vector<float> JsonRpcTransport::ParseResponse(const std::string& response) {
    // Simplified JSON parsing (would use nlohmann/json in production)
    std::vector<float> result;
    
    // Mock: extract [1.0, 2.0, 3.0]
    result.push_back(1.0f);
    result.push_back(2.0f);
    result.push_back(3.0f);
    
    return result;
}

std::map<std::string, std::string> JsonRpcTransport::BatchSend(
    const std::vector<std::string>& endpoints,
    const std::string& method,
    const std::map<std::string, std::string>& params) {
    
    std::map<std::string, std::string> responses;
    std::vector<std::future<std::string>> futures;
    
    // Launch parallel requests
    for (const auto& endpoint : endpoints) {
        auto task = std::async(std::launch::async,
            [endpoint, method, &params]() {
                return SendRequest(endpoint, method, params);
            });
        futures.push_back(std::move(task));
    }
    
    // Collect results
    for (size_t i = 0; i < futures.size(); i++) {
        responses[endpoints[i]] = futures[i].get();
    }
    
    return responses;
}

} // namespace Distributed
} // namespace CanvasX
