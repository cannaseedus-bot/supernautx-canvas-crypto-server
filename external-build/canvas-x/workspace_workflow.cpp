/// workspace_workflow.cpp
/// Workflow execution implementation

#include "workspace_workflow.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>

// ============================================================================
// WORKFLOW EXECUTOR
// ============================================================================

WorkflowExecutor::WorkflowExecutor() {
    // Register built-in operations
    RegisterOpHandler("fs.read", [this](const auto& params) {
        auto it = params.find("path");
        return it != params.end() ? OpFsRead(it->second) : "ERROR: path not specified";
    });

    RegisterOpHandler("fs.write", [this](const auto& params) {
        auto pathIt = params.find("path");
        auto contentIt = params.find("content");
        return (pathIt != params.end() && contentIt != params.end()) 
            ? OpFsWrite(pathIt->second, contentIt->second) 
            : "ERROR: path or content not specified";
    });

    RegisterOpHandler("moe.infer", [this](const auto& params) {
        auto it = params.find("input");
        return it != params.end() ? OpMoeInfer(it->second) : "ERROR: input not specified";
    });

    RegisterOpHandler("http.call", [this](const auto& params) {
        auto urlIt = params.find("url");
        auto bodyIt = params.find("body");
        return (urlIt != params.end() && bodyIt != params.end())
            ? OpHttpCall(urlIt->second, bodyIt->second)
            : "ERROR: url or body not specified";
    });
}

WorkflowExecutor::~WorkflowExecutor() {}

std::string WorkflowExecutor::ExecuteWorkflow(const std::string& workflowJson) {
    std::vector<WorkflowOp> ops = ParseWorkflow(workflowJson);
    std::string context = "";

    for (const auto& op : ops) {
        context = ExecuteOp(op, context);
    }

    return context;
}

void WorkflowExecutor::RegisterOpHandler(
    const std::string& opName,
    std::function<std::string(const std::map<std::string, std::string>&)> handler
) {
    opHandlers_[opName] = handler;
}

std::string WorkflowExecutor::OpFsRead(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        file.close();
        return ss.str();
    }
    return "ERROR: file not found";
}

std::string WorkflowExecutor::OpFsWrite(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (file.is_open()) {
        file << content;
        file.close();
        return "OK";
    }
    return "ERROR: cannot write file";
}

std::string WorkflowExecutor::OpMoeInfer(const std::string& input) {
    // Stub for Phase 7.20
    return "inference_result: " + input;
}

std::string WorkflowExecutor::OpHttpCall(const std::string& url, const std::string& body) {
    // Stub for Phase 7.20 (integrate with HTTP client)
    return "http_response: OK";
}

std::vector<WorkflowOp> WorkflowExecutor::ParseWorkflow(const std::string& json) {
    std::vector<WorkflowOp> ops;

    // Simple CSV-like parsing (MVP - full JSON parser in Phase 7.20)
    // Expected format: [{"@op":"fs.read","path":"file.txt"}, ...]
    
    size_t pos = 0;
    while ((pos = json.find("@op", pos)) != std::string::npos) {
        WorkflowOp op;
        
        // Extract op name
        size_t opStart = json.find(":", pos) + 1;
        size_t opEnd = json.find("\"", opStart + 1);
        op.op = json.substr(opStart + 1, opEnd - opStart - 1);

        ops.push_back(op);
        pos = opEnd;
    }

    return ops;
}

std::string WorkflowExecutor::ExecuteOp(const WorkflowOp& op, const std::string& context) {
    auto it = opHandlers_.find(op.op);
    if (it != opHandlers_.end()) {
        return it->second(op.params);
    }
    return "ERROR: unknown operation " + op.op;
}

// ============================================================================
// WORKFLOW STATE
// ============================================================================

void WorkflowState::LoadWorkflow(const std::string& name, const std::string& json) {
    workflows.push_back(name);
}

void WorkflowState::RunWorkflow(const std::string& name) {
    currentWorkflow = name;
    isExecuting = true;
    
    // Find and execute workflow
    // For MVP, just stub
    workflowOutput = "Workflow " + name + " executed";
    
    isExecuting = false;
}

void WorkflowState::StopWorkflow() {
    isExecuting = false;
}
