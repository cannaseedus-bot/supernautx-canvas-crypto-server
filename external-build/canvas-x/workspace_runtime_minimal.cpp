/// workspace_runtime.cpp
/// Minimal implementation of unified workspace runtime

#include "workspace_runtime.hpp"
#include "http_client_native.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

// ============================================================================
// EDITOR STATE IMPLEMENTATION (Minimal Stubs)
// ============================================================================

void EditorState::InsertChar(char c) {
    if (cursorY >= lines.size()) lines.push_back("");
    lines[cursorY].insert(cursorX, 1, c);
    cursorX++;
    isModified = true;
}

void EditorState::DeleteChar() {
    if (cursorY < lines.size() && cursorX < lines[cursorY].size()) {
        lines[cursorY].erase(cursorX, 1);
        isModified = true;
    }
}

void EditorState::NewLine() {
    if (cursorY >= lines.size()) lines.push_back("");
    std::string remainder = lines[cursorY].substr(cursorX);
    lines[cursorY] = lines[cursorY].substr(0, cursorX);
    lines.insert(lines.begin() + cursorY + 1, remainder);
    cursorY++;
    cursorX = 0;
    isModified = true;
}

void EditorState::Backspace() {
    if (cursorX > 0) {
        cursorX--;
        DeleteChar();
    }
}

void EditorState::MoveCursor(int dx, int dy) {
    cursorX += dx;
    cursorY += dy;
    if (cursorY < 0) cursorY = 0;
    if (cursorY >= lines.size()) cursorY = lines.size() - 1;
    if (cursorX < 0) cursorX = 0;
}

void EditorState::ScrollView(int delta) {
    scrollY += delta;
    if (scrollY < 0) scrollY = 0;
}

// ============================================================================
// CHAT STATE IMPLEMENTATION
// ============================================================================

void ChatState::AddMessage(std::string role, std::string text) {
    messages.push_back({role, text, 0});
}

void ChatState::ClearInput() {
    currentInput = "";
}

// ============================================================================
// CANVAS STATE IMPLEMENTATION
// ============================================================================

void CanvasState::UpdateSource(std::string src) {
    sourceCode = src;
    isStale = true;
}

void CanvasState::Execute() {
    executeResult = "preview";
    isStale = false;
}

// ============================================================================
// WORKSPACE RUNTIME IMPLEMENTATION
// ============================================================================

WorkspaceRuntime::WorkspaceRuntime() {
    editor.lines.push_back("");
}

WorkspaceRuntime::~WorkspaceRuntime() {}

void WorkspaceRuntime::Initialize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    isInitialized = true;
}

void WorkspaceRuntime::Shutdown() {
    isInitialized = false;
}

void WorkspaceRuntime::Update(float deltaTime) {
    // Process pending events
}

void WorkspaceRuntime::Render(void* renderContext) {
    // Render all 8 subsystems
}

void WorkspaceRuntime::OnKeyPress(int vkey) {
    if (vkey >= 32 && vkey <= 126) {
        editor.InsertChar((char)vkey);
    } else if (vkey == 8) {  // Backspace
        editor.Backspace();
    } else if (vkey == 13) {  // Enter
        editor.NewLine();
    } else if (vkey == 37) {  // Left
        editor.MoveCursor(-1, 0);
    } else if (vkey == 39) {  // Right
        editor.MoveCursor(1, 0);
    } else if (vkey == 38) {  // Up
        editor.MoveCursor(0, -1);
    } else if (vkey == 40) {  // Down
        editor.MoveCursor(0, 1);
    }
    CaptureState("editor-edit");
}

void WorkspaceRuntime::OnKeyRelease(int vkey) {}

void WorkspaceRuntime::OnMouseClick(int x, int y, int button) {}

void WorkspaceRuntime::OnMouseMove(int x, int y) {}

void WorkspaceRuntime::OnMouseWheel(int x, int y, int delta) {}

void WorkspaceRuntime::OnChar(wchar_t c) {}

void WorkspaceRuntime::OnUserPrompt(std::string input) {
    // Dispatch to agent with HTTP request
    DispatchAgentRequest(input);
}

void WorkspaceRuntime::OnFileOpen(std::string path) {
    editor.filepath = path;
    CaptureState("file-open");
}

void WorkspaceRuntime::OnExecuteCode(std::string code) {
    editor.lines.push_back(code);
    canvas.Execute();
    CaptureState("code-execution");
}

void WorkspaceRuntime::OnRunWorkflow(std::string workflowName) {
    for (auto& wf : workflows.workflows) {
        if (wf == workflowName) {
            workflows.ExecuteWorkflow(wf);
            break;
        }
    }
}

void WorkspaceRuntime::CaptureState(std::string action) {
    // Minimal implementation - just track action
}

void WorkspaceRuntime::RestoreState(int frameIdx) {
    if (frameIdx >= 0 && frameIdx < timeline.frames.size()) {
        timeline.currentFrame = frameIdx;
    }
}

Rect WorkspaceRuntime::GetPanelBounds(std::string panelName) {
    return {0, 0, 100, 100};
}

int WorkspaceRuntime::GetPanelAtPoint(int x, int y) {
    return 0;
}

void WorkspaceRuntime::HandlePanelInput(int panel, int vkey) {}

// ============================================================================
// AGENT DISPATCH IMPLEMENTATION
// ============================================================================

void WorkspaceRuntime::DispatchAgentRequest(std::string userInput) {
    if (userInput.empty()) return;
    
    // Add user message to chat
    chat.AddMessage("user", userInput);
    chat.isAwaitingResponse = true;
    CaptureState("chat-agent-dispatch");
    
    // Dispatch to agent in background thread
    std::thread([this, userInput]() {
        AgentDispatcher dispatcher;
        std::string response = dispatcher.CallAgent(chat.agentUrl, userInput);
        chat.lastAgentResponse = response;
        chat.AddMessage("agent", response);
        chat.isAwaitingResponse = false;
    }).detach();
}

std::string WorkspaceRuntime::GetLastAgentResponse() {
    return chat.lastAgentResponse;
}

void WorkflowState::LoadWorkflows(std::string configPath) {
    // Minimal implementation
}

std::string WorkflowState::ExecuteWorkflow(std::string wf, std::string initialData) {
    executionLog.push_back("Executed: " + wf);
    return "done";
}

// ============================================================================
// EXECUTION ENGINE IMPLEMENTATION
// ============================================================================

std::string ExecutionEngine::ExecuteJSON(std::string node) {
    return "{}";
}

std::string ExecutionEngine::ExecuteWorkflow(std::vector<std::string> steps, std::string initialData) {
    return "done";
}

std::string ExecutionEngine::OpFileRead(std::string path) {
    return "";
}

void ExecutionEngine::OpFileWrite(std::string path, std::string content) {}

std::string ExecutionEngine::OpAgentRun(std::string agentName, std::string input) {
    return "agent-response";
}

std::string ExecutionEngine::OpTerminalExec(std::string cmd) {
    return "command-output";
}

std::string ExecutionEngine::OpCanvasRender(std::string block) {
    return "svg-output";
}
