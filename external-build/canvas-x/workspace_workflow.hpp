/// workspace_workflow.hpp
/// JSON.X workflow execution engine

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// ============================================================================
// WORKFLOW OPERATION
// ============================================================================

struct WorkflowOp {
    std::string op;  // fs.read, fs.write, moe.infer, etc.
    std::map<std::string, std::string> params;
    std::string result;
};

// ============================================================================
// WORKFLOW EXECUTOR
// ============================================================================

class WorkflowExecutor {
public:
    WorkflowExecutor();
    ~WorkflowExecutor();

    // Parse and execute workflow
    std::string ExecuteWorkflow(const std::string& workflowJson);

    // Register operation handler
    void RegisterOpHandler(
        const std::string& opName,
        std::function<std::string(const std::map<std::string, std::string>&)> handler
    );

    // Built-in operations
    std::string OpFsRead(const std::string& path);
    std::string OpFsWrite(const std::string& path, const std::string& content);
    std::string OpMoeInfer(const std::string& input);
    std::string OpHttpCall(const std::string& url, const std::string& body);

private:
    std::map<std::string, std::function<std::string(const std::map<std::string, std::string>&)>> opHandlers_;

    std::vector<WorkflowOp> ParseWorkflow(const std::string& json);
    std::string ExecuteOp(const WorkflowOp& op, const std::string& context);
};

// ============================================================================
// WORKFLOW STATE
// ============================================================================

struct WorkflowState {
    std::vector<std::string> workflows;
    std::string currentWorkflow = "";
    std::string workflowOutput = "";
    bool isExecuting = false;
    WorkflowExecutor executor;

    void LoadWorkflow(const std::string& name, const std::string& json);
    void RunWorkflow(const std::string& name);
    void StopWorkflow();
};
