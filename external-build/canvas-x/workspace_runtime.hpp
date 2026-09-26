/// workspace_runtime.hpp
/// Core unified workspace runtime (Canvas.X + Editor + Chat + Agents)

#pragma once

#include <vector>
#include <string>
#include <map>
#include <chrono>

// Use simple JSON typedef for now (Phase 7.20 can add full JSON support)
using json = std::map<std::string, std::string>;

// ============================================================================
// EDITOR STATE
// ============================================================================

struct EditorState {
    std::vector<std::string> lines;
    int cursorX = 0;
    int cursorY = 0;
    int scrollY = 0;
    std::string filepath = "";
    bool isModified = false;
    
    void InsertChar(char c);
    void DeleteChar();
    void NewLine();
    void Backspace();
    void MoveCursor(int dx, int dy);
    void ScrollView(int delta);
};

// ============================================================================
// CHAT STATE
// ============================================================================

struct Message {
    std::string role;  // "user" or "agent"
    std::string text;
    int64_t timestamp = 0;
};

struct ChatState {
    std::vector<Message> messages;
    std::string currentInput = "";
    int scrollY = 0;
    bool isAwaitingResponse = false;
    std::string agentUrl = "http://localhost:5775/agent";  // Default agent endpoint
    std::string lastAgentResponse = "";
    
    void AddMessage(std::string role, std::string text);
    void ClearInput();
};

// ============================================================================
// FILE SYSTEM STATE
// ============================================================================

struct FileNode {
    std::string path;
    std::string name;
    bool isDir = false;
    std::vector<FileNode> children;
    bool isExpanded = false;
    bool isModified = false;
};

struct FileSystemState {
    std::string rootPath = "./project";
    std::vector<FileNode> tree;
    std::string selectedFile = "";
    int scrollY = 0;
    
    void LoadTree(std::string path);
    std::string ReadFile(std::string path);
    void WriteFile(std::string path, std::vector<std::string> lines);
};

// ============================================================================
// TERMINAL STATE
// ============================================================================

struct TerminalState {
    std::vector<std::string> history;
    std::string currentCommand = "";
    int scrollY = 0;
    bool isRunning = false;
    
    void ExecuteCommand(std::string cmd);
    void AddOutput(std::string line);
};

// ============================================================================
// CANVAS STATE
// ============================================================================

struct CanvasState {
    std::string sourceCode = "";
    std::string executeResult;
    std::string errorMessage = "";
    bool isStale = false;
    bool isVisible = false;
    
    void UpdateSource(std::string src);
    void Execute();
};

// ============================================================================
// TIMELINE STATE
// ============================================================================

struct Frame {
    std::string state;  // Full snapshot: editor + chat + files
    std::string actionName;
    int64_t timestamp = 0;
};

struct TimelineState {
    std::vector<Frame> frames;
    int currentFrame = 0;
    bool isPlaying = false;
    float playbackSpeed = 1.0f;
    
    void CaptureFrame(std::string action);
    void RestoreFrame(int idx);
    void SeekFrame(int idx);
    void Play();
    void Pause();
};

// ============================================================================
// AGENT STATE
// ============================================================================

struct Agent {
    std::string name;
    std::vector<std::string> skills;
    std::string model;
    int port = 5775;
    std::string description = "";
};

struct AgentState {
    std::vector<Agent> agents;
    std::string selectedAgent = "";
    
    void LoadAgents(std::string configPath);
    std::string RunAgent(std::string agentName, std::string input);
};

// ============================================================================
// WORKFLOW STATE
// ============================================================================

struct WorkflowState {
    std::vector<std::string> workflows;
    std::string selectedWorkflow = "";
    std::vector<std::string> executionLog;
    
    void LoadWorkflows(std::string configPath);
    std::string ExecuteWorkflow(std::string wf, std::string initialData = "");
};

// ============================================================================
// LAYOUT & RENDERING
// ============================================================================

struct Rect {
    float x, y, w, h;
};

struct LayoutGrid {
    Rect left;      // Sidebar (240px)
    Rect center;    // Editor (1024px)
    Rect right;     // Chat (320px)
    Rect bottom;    // Terminal (200px)
    Rect preview;   // Canvas preview (overlay)
};

// ============================================================================
// MAIN WORKSPACE RUNTIME
// ============================================================================

class WorkspaceRuntime {
public:
    // State
    EditorState editor;
    ChatState chat;
    FileSystemState files;
    TerminalState terminal;
    CanvasState canvas;
    TimelineState timeline;
    AgentState agents;
    WorkflowState workflows;
    
    LayoutGrid layout;
    
    // Lifecycle
    WorkspaceRuntime();
    ~WorkspaceRuntime();
    void Initialize(int width = 1584, int height = 768);
    void Shutdown();
    
    // Main loop
    void Update(float deltaTime);
    void Render(void* renderContext);  // RenderContext from Canvas.X
    
    // Input
    void OnKeyPress(int vkey);
    void OnKeyRelease(int vkey);
    void OnMouseClick(int x, int y, int button);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int x, int y, int delta);
    void OnChar(wchar_t c);
    
    // Pipelines
    void OnUserPrompt(std::string input);
    void OnFileOpen(std::string path);
    void OnExecuteCode(std::string code);
    void OnRunWorkflow(std::string workflowName);
    
    // Agent dispatch
    void DispatchAgentRequest(std::string userInput);
    std::string GetLastAgentResponse();
    
    // State capture
    void CaptureState(std::string action);
    void RestoreState(int frameIdx);
    
private:
    bool isInitialized = false;
    int windowWidth = 1584;
    int windowHeight = 768;
    
    // Helpers
    Rect GetPanelBounds(std::string panelName);
    int GetPanelAtPoint(int x, int y);
    void HandlePanelInput(int panel, int vkey);
};

// ============================================================================
// EXECUTION ENGINE
// ============================================================================

class ExecutionEngine {
public:
    std::string ExecuteJSON(std::string node);
    std::string ExecuteWorkflow(std::vector<std::string> steps, std::string initialData);
    
private:
    // Operation handlers
    std::string OpFileRead(std::string path);
    void OpFileWrite(std::string path, std::string content);
    std::string OpAgentRun(std::string agentName, std::string input);
    std::string OpTerminalExec(std::string cmd);
    std::string OpCanvasRender(std::string block);
};
