/// workspace_runtime.cpp
/// Implementation of unified workspace runtime

#include "workspace_runtime.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

// ============================================================================
// EDITOR STATE IMPLEMENTATION
// ============================================================================

void EditorState::InsertChar(char c) {
    if (cursorY < 0 || cursorY >= lines.size()) return;
    lines[cursorY].insert(cursorX, 1, c);
    cursorX++;
    isModified = true;
}

void EditorState::DeleteChar() {
    if (cursorY < 0 || cursorY >= lines.size()) return;
    if (cursorX > 0 && cursorX <= lines[cursorY].size()) {
        lines[cursorY].erase(cursorX - 1, 1);
        cursorX--;
        isModified = true;
    }
}

void EditorState::NewLine() {
    if (cursorY < 0 || cursorY >= lines.size()) return;
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
    } else if (cursorY > 0) {
        // Move to end of previous line and merge
        cursorY--;
        cursorX = lines[cursorY].size();
        lines[cursorY] += lines[cursorY + 1];
        lines.erase(lines.begin() + cursorY + 1);
        isModified = true;
    }
}

void EditorState::MoveCursor(int dx, int dy) {
    cursorX += dx;
    cursorY += dy;
    
    // Clamp
    if (cursorY < 0) cursorY = 0;
    if (cursorY >= lines.size()) cursorY = lines.size() - 1;
    if (cursorX < 0) cursorX = 0;
    if (cursorY < lines.size() && cursorX > lines[cursorY].size()) {
        cursorX = lines[cursorY].size();
    }
}

void EditorState::ScrollView(int delta) {
    scrollY += delta;
    if (scrollY < 0) scrollY = 0;
}

// ============================================================================
// CHAT STATE IMPLEMENTATION
// ============================================================================

void ChatState::AddMessage(std::string role, std::string text) {
    Message m;
    m.role = role;
    m.text = text;
    m.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    messages.push_back(m);
}

void ChatState::ClearInput() {
    currentInput = "";
}

// ============================================================================
// FILE SYSTEM STATE IMPLEMENTATION
// ============================================================================

void FileSystemState::LoadTree(std::string path) {
    rootPath = path;
    tree.clear();
    
    namespace fs = std::filesystem;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            FileNode node;
            node.path = entry.path().string();
            node.name = entry.path().filename().string();
            node.isDir = fs::is_directory(entry);
            tree.push_back(node);
        }
    } catch (std::exception& e) {
        // Handle error
    }
}

std::string FileSystemState::ReadFile(std::string path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void FileSystemState::WriteFile(std::string path, std::vector<std::string> lines) {
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    for (const auto& line : lines) {
        file << line << "\n";
    }
    file.close();
}

// ============================================================================
// TERMINAL STATE IMPLEMENTATION
// ============================================================================

void TerminalState::ExecuteCommand(std::string cmd) {
    currentCommand = cmd;
    isRunning = true;
    
    // Execute and capture output (platform-specific)
    #ifdef _WIN32
        FILE* pipe = _popen(cmd.c_str(), "r");
    #else
        FILE* pipe = popen(cmd.c_str(), "r");
    #endif
    
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            AddOutput(buffer);
        }
        #ifdef _WIN32
            _pclose(pipe);
        #else
            pclose(pipe);
        #endif
    }
    
    isRunning = false;
}

void TerminalState::AddOutput(std::string line) {
    history.push_back(line);
}

// ============================================================================
// CANVAS STATE IMPLEMENTATION
// ============================================================================

void CanvasState::UpdateSource(std::string src) {
    sourceCode = src;
    isStale = true;
}

void CanvasState::Execute() {
    if (!isStale || sourceCode.empty()) return;
    
    try {
        executeResult = Json::Value();  // Would call ExecutionEngine here
        errorMessage = "";
    } catch (std::exception& e) {
        errorMessage = e.what();
    }
    
    isStale = false;
}

// ============================================================================
// TIMELINE STATE IMPLEMENTATION
// ============================================================================

void TimelineState::CaptureFrame(std::string action) {
    Frame f;
    f.actionName = action;
    f.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Capture current state (would serialize here)
    frames.push_back(f);
    currentFrame = frames.size() - 1;
}

void TimelineState::RestoreFrame(int idx) {
    if (idx >= 0 && idx < frames.size()) {
        currentFrame = idx;
        // Would deserialize state here
    }
}

void TimelineState::SeekFrame(int idx) {
    RestoreFrame(idx);
}

void TimelineState::Play() {
    isPlaying = true;
}

void TimelineState::Pause() {
    isPlaying = false;
}

// ============================================================================
// AGENT STATE IMPLEMENTATION
// ============================================================================

void AgentState::LoadAgents(std::string configPath) {
    // Load agent configs from JSON
    std::ifstream file(configPath);
    if (!file.is_open()) return;
    
    Json::Value root;
    file >> root;
    
    for (const auto& agentJson : root["agents"]) {
        Agent a;
        a.name = agentJson["name"].asString();
        a.model = agentJson["model"].asString();
        a.port = agentJson.get("port", 5775).asInt();
        
        for (const auto& skill : agentJson["skills"]) {
            a.skills.push_back(skill.asString());
        }
        
        agents.push_back(a);
    }
}

std::string AgentState::RunAgent(std::string agentName, std::string input) {
    // Find agent
    for (auto& a : agents) {
        if (a.name == agentName) {
            // Call HTTP endpoint (would integrate with HTTPClient here)
            // POST to http://localhost:{port}/generate with input
            // Return response
            return "/* Generated code from " + agentName + " */";
        }
    }
    return "";
}

// ============================================================================
// WORKFLOW STATE IMPLEMENTATION
// ============================================================================

void WorkflowState::LoadWorkflows(std::string configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) return;
    
    Json::Value root;
    file >> root;
    
    for (const auto& wf : root["workflows"]) {
        workflows.push_back(wf);
    }
}

Json::Value WorkflowState::ExecuteWorkflow(Json::Value wf, Json::Value initialData) {
    Json::Value data = initialData;
    
    for (const auto& step : wf["steps"]) {
        std::string op = step["@op"].asString();
        
        executionLog.push_back("Executing: " + op);
        
        // Dispatch to operations
        if (op == "fs.read") {
            // data = FileRead(step["path"])
        } else if (op == "fs.write") {
            // FileWrite(step["path"], data)
        } else if (op == "agent.run") {
            // data = RunAgent(step["agent"], data)
        }
    }
    
    return data;
}

// ============================================================================
// WORKSPACE RUNTIME IMPLEMENTATION
// ============================================================================

void WorkspaceRuntime::Initialize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    
    // Initialize layout grid
    layout.left = {0, 0, 240, (float)height - 200};
    layout.center = {240, 0, 1024, (float)height - 200};
    layout.right = {1264, 0, 320, (float)height - 200};
    layout.bottom = {0, (float)height - 200, (float)width, 200};
    layout.preview = {0, 0, (float)width, (float)height};
    
    // Initialize editor
    editor.lines.push_back("");  // Start with one empty line
    
    // Load agents and workflows
    agents.LoadAgents("config/agents.json");
    workflows.LoadWorkflows("config/workflows.json");
    
    isInitialized = true;
}

void WorkspaceRuntime::Shutdown() {
    isInitialized = false;
}

void WorkspaceRuntime::Update(float deltaTime) {
    // Update canvas if stale
    canvas.Execute();
    
    // Update timeline playback
    if (timeline.isPlaying) {
        // Advance frame
    }
}

void WorkspaceRuntime::Render(void* renderContext) {
    // Render all panels in layout
    // RenderEditor(rc, editor, layout.center);
    // RenderChat(rc, chat, layout.right);
    // RenderFiles(rc, files, layout.left);
    // RenderTerminal(rc, terminal, layout.bottom);
    // if (canvas.isVisible) RenderCanvas(rc, canvas, layout.preview);
}

void WorkspaceRuntime::OnKeyPress(int vkey) {
    if (vkey >= 0x20 && vkey <= 0x7E) {
        editor.InsertChar((char)vkey);
    } else if (vkey == VK_DELETE) {
        editor.DeleteChar();
    } else if (vkey == VK_BACK) {
        editor.Backspace();
    } else if (vkey == VK_RETURN) {
        editor.NewLine();
    } else if (vkey == VK_LEFT) {
        editor.MoveCursor(-1, 0);
    } else if (vkey == VK_RIGHT) {
        editor.MoveCursor(1, 0);
    } else if (vkey == VK_UP) {
        editor.MoveCursor(0, -1);
    } else if (vkey == VK_DOWN) {
        editor.MoveCursor(0, 1);
    }
}

void WorkspaceRuntime::OnKeyRelease(int vkey) {
    // Handle key releases
}

void WorkspaceRuntime::OnMouseClick(int x, int y, int button) {
    // Determine which panel was clicked
    int panel = GetPanelAtPoint(x, y);
    
    // Update selected state in that panel
    if (panel == 0) {  // Files
        // Handle file selection
    } else if (panel == 1) {  // Editor
        // Update cursor position
    } else if (panel == 2) {  // Chat
        // Submit message if in input area
    }
}

void WorkspaceRuntime::OnMouseMove(int x, int y) {
    // Update cursor position or hovering state
}

void WorkspaceRuntime::OnMouseWheel(int x, int y, int delta) {
    // Scroll appropriate panel
    int panel = GetPanelAtPoint(x, y);
    if (panel == 1) {  // Editor
        editor.ScrollView(delta > 0 ? -3 : 3);
    } else if (panel == 2) {  // Chat
        chat.scrollY += delta > 0 ? -3 : 3;
    }
}

void WorkspaceRuntime::OnChar(wchar_t c) {
    if (c >= 32 && c < 127) {
        editor.InsertChar((char)c);
    }
}

void WorkspaceRuntime::OnUserPrompt(std::string input) {
    // Add user message to chat
    chat.AddMessage("user", input);
    chat.isAwaitingResponse = true;
    
    // Run agent
    std::string agentName = agents.selectedAgent.empty() ? "default" : agents.selectedAgent;
    std::string response = agents.RunAgent(agentName, input);
    
    // Add response to chat
    chat.AddMessage("agent", response);
    
    // Append to editor
    for (const auto& line : response) {
        if (line == '\n') {
            editor.NewLine();
        } else {
            editor.InsertChar(line);
        }
    }
    
    // Capture state
    CaptureState("agent-generation");
    
    chat.isAwaitingResponse = false;
}

void WorkspaceRuntime::OnFileOpen(std::string path) {
    std::string content = files.ReadFile(path);
    editor.lines.clear();
    
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        editor.lines.push_back(line);
    }
    
    editor.filepath = path;
    editor.cursorX = 0;
    editor.cursorY = 0;
    editor.isModified = false;
}

void WorkspaceRuntime::OnExecuteCode(std::string code) {
    terminal.ExecuteCommand(code);
    CaptureState("code-execution");
}

void WorkspaceRuntime::OnRunWorkflow(std::string workflowName) {
    for (auto& wf : workflows.workflows) {
        // Since workflows are now strings, skip complex parsing for MVP
        if (wf == workflowName) {
            workflows.ExecuteWorkflow(wf);
            break;
        }
    }
}

void WorkspaceRuntime::CaptureState(std::string action) {
    timeline.CaptureFrame(action);
}

void WorkspaceRuntime::RestoreState(int frameIdx) {
    timeline.RestoreFrame(frameIdx);
}

Rect WorkspaceRuntime::GetPanelBounds(std::string panelName) {
    if (panelName == "left") return layout.left;
    if (panelName == "center") return layout.center;
    if (panelName == "right") return layout.right;
    if (panelName == "bottom") return layout.bottom;
    return {};
}

int WorkspaceRuntime::GetPanelAtPoint(int x, int y) {
    if (x >= layout.left.x && x < layout.left.x + layout.left.w) return 0;  // Files
    if (x >= layout.center.x && x < layout.center.x + layout.center.w) return 1;  // Editor
    if (x >= layout.right.x && x < layout.right.x + layout.right.w) return 2;  // Chat
    if (y >= layout.bottom.y) return 3;  // Terminal
    return -1;
}

void WorkspaceRuntime::HandlePanelInput(int panel, int vkey) {
    // Route input to specific panel handlers
}
