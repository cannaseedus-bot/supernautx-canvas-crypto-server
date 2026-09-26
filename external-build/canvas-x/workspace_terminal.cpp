/// workspace_terminal.cpp
/// Process execution implementation

#include "workspace_terminal.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>

// ============================================================================
// PROCESS EXECUTOR
// ============================================================================

ProcessExecutor::ProcessExecutor() {}

ProcessExecutor::~ProcessExecutor() {
    Terminate();
}

ProcessOutput ProcessExecutor::ExecuteCommand(const std::string& command) {
    return ExecuteInternal(command);
}

void ProcessExecutor::ExecuteCommandAsync(
    const std::string& command,
    std::function<void(const ProcessOutput&)> onComplete
) {
    struct AsyncContext {
        std::string cmd;
        std::function<void(const ProcessOutput&)> callback;
        ProcessExecutor* executor;
    };

    AsyncContext* ctx = new AsyncContext{command, onComplete, this};
    
    threadHandle_ = CreateThread(
        NULL,
        0,
        AsyncThreadProc,
        ctx,
        0,
        NULL
    );
}

bool ProcessExecutor::IsRunning() const {
    return isRunning_;
}

void ProcessExecutor::Terminate() {
    if (processHandle_ != INVALID_HANDLE_VALUE) {
        TerminateProcess(processHandle_, 1);
        CloseHandle(processHandle_);
        processHandle_ = INVALID_HANDLE_VALUE;
    }

    if (threadHandle_ != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(threadHandle_, 5000);
        CloseHandle(threadHandle_);
        threadHandle_ = INVALID_HANDLE_VALUE;
    }

    isRunning_ = false;
}

ProcessOutput ProcessExecutor::ExecuteInternal(const std::string& command) {
    ProcessOutput output;
    output.start_time = std::chrono::system_clock::now().time_since_epoch().count();

    // Create pipes for stdout/stderr
    HANDLE stdoutRead, stdoutWrite, stderrRead, stderrWrite;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&stdoutRead, &stdoutWrite, &saAttr, 0) ||
        !CreatePipe(&stderrRead, &stderrWrite, &saAttr, 0)) {
        output.exit_code = -1;
        output.stderr_text = "Failed to create pipes";
        return output;
    }

    // Make inheritable ends non-inheritable
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    // Create process
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA siStartInfo;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdOutput = stdoutWrite;
    siStartInfo.hStdError = stderrWrite;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Convert command to non-const for CreateProcessA
    std::string cmdCopy = command;

    isRunning_ = CreateProcessA(
        NULL,
        (LPSTR)cmdCopy.c_str(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo
    );

    if (!isRunning_) {
        output.exit_code = -1;
        output.stderr_text = "Failed to create process";
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
        return output;
    }

    processHandle_ = piProcInfo.hProcess;
    CloseHandle(piProcInfo.hThread);

    // Close child-side handles
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    // Read output
    char buffer[4096];
    DWORD bytesRead = 0;

    while (ReadFile(stdoutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output.stdout_text += buffer;
    }

    while (ReadFile(stderrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output.stderr_text += buffer;
    }

    // Wait for process
    WaitForSingleObject(processHandle_, INFINITE);

    // Get exit code
    GetExitCodeProcess(processHandle_, (LPDWORD)&output.exit_code);

    CloseHandle(processHandle_);
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);

    processHandle_ = INVALID_HANDLE_VALUE;
    isRunning_ = false;

    output.end_time = std::chrono::system_clock::now().time_since_epoch().count();
    output.completed = true;

    return output;
}

DWORD WINAPI ProcessExecutor::AsyncThreadProc(LPVOID param) {
    struct AsyncContext {
        std::string cmd;
        std::function<void(const ProcessOutput&)> callback;
        ProcessExecutor* executor;
    };

    AsyncContext* ctx = (AsyncContext*)param;
    ProcessOutput result = ctx->executor->ExecuteInternal(ctx->cmd);
    
    if (ctx->callback) {
        ctx->callback(result);
    }

    delete ctx;
    return 0;
}

// ============================================================================
// TERMINAL STATE
// ============================================================================

void TerminalState::ExecuteCommand(std::string cmd) {
    currentCommand = cmd;
    lastCommandTime = std::chrono::system_clock::now().time_since_epoch().count();
    isRunning = true;

    history.push_back(cmd);

    // Execute synchronously for MVP (Phase 7.20 can make async)
    ProcessOutput result = executor.ExecuteCommand(cmd);
    
    AddOutput("$ " + cmd);
    AddOutput(result.stdout_text);
    if (!result.stderr_text.empty()) {
        AddOutput("ERROR: " + result.stderr_text);
    }
    AddOutput("Exit code: " + std::to_string(result.exit_code));

    isRunning = false;
}

void TerminalState::AddOutput(std::string line) {
    output += line + "\n";
}

void TerminalState::ClearOutput() {
    output.clear();
}

void TerminalState::ScrollUp() {
    if (scrollY > 3) {
        scrollY -= 3;
    } else {
        scrollY = 0;
    }
}

void TerminalState::ScrollDown() {
    scrollY += 3;
}
