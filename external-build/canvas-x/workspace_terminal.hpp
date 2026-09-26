/// workspace_terminal.hpp
/// Native process execution with pipe redirection

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <windows.h>

// ============================================================================
// PROCESS OUTPUT STREAM
// ============================================================================

struct ProcessOutput {
    std::string stdout_text;
    std::string stderr_text;
    int exit_code = -1;
    bool completed = false;
    int64_t start_time = 0;
    int64_t end_time = 0;
};

// ============================================================================
// PROCESS EXECUTOR
// ============================================================================

class ProcessExecutor {
public:
    ProcessExecutor();
    ~ProcessExecutor();

    // Execute command and wait for completion
    ProcessOutput ExecuteCommand(const std::string& command);

    // Execute command asynchronously with callback
    void ExecuteCommandAsync(
        const std::string& command,
        std::function<void(const ProcessOutput&)> onComplete
    );

    // Check if process is still running
    bool IsRunning() const;

    // Terminate current process
    void Terminate();

private:
    HANDLE processHandle_ = INVALID_HANDLE_VALUE;
    HANDLE threadHandle_ = INVALID_HANDLE_VALUE;
    bool isRunning_ = false;

    ProcessOutput ExecuteInternal(const std::string& command);
    static DWORD WINAPI AsyncThreadProc(LPVOID param);
};

// ============================================================================
// TERMINAL STATE
// ============================================================================

struct TerminalState {
    std::vector<std::string> history;
    std::string currentCommand = "";
    std::string output = "";
    int scrollY = 0;
    bool isRunning = false;
    ProcessExecutor executor;
    int64_t lastCommandTime = 0;

    void ExecuteCommand(std::string cmd);
    void AddOutput(std::string line);
    void ClearOutput();
    void ScrollUp();
    void ScrollDown();
};
