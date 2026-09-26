/// workspace_runtime_integration.cpp
/// Workspace Runtime ↔ Canvas Integration Bridge
/// Connects WorkspaceRuntime orchestrator to D2D rendering subsystem

#include "workspace_runtime.hpp"
#include "workspace_canvas_integration.hpp"

// ============================================================================
// WORKSPACE RUNTIME CANVAS INTEGRATION
// ============================================================================

/// Initialize workspace with canvas backend
bool WorkspaceRuntime::InitializeWithCanvas(HWND hwnd, int width, int height) {
    canvasIntegration = std::make_unique<WorkspaceCanvasIntegration>(this);
    
    if (!canvasIntegration->Initialize(hwnd, width, height)) {
        LogError("Failed to initialize canvas integration");
        return false;
    }
    
    // Initialize all subsystem states
    editorState.Clear();
    chatState.messages.clear();
    fileSystemState.currentDir = ".";
    
    // Load initial project tree
    fileSystemState.LoadTree(fileSystemState.currentDir);
    
    // Load sample agent config
    LoadAgentConfig();
    
    isRunning = true;
    return true;
}

/// Main update loop - call every frame
void WorkspaceRuntime::UpdateFrame(double deltaTime) {
    if (!isRunning) return;
    
    // Update subsystem states
    editorState.Update(deltaTime);
    
    // Timeline capture (every 100ms)
    timelineState.frameTime += deltaTime;
    if (timelineState.frameTime >= 0.1) {
        CaptureTimelineFrame();
        timelineState.frameTime = 0.0;
    }
    
    // Update terminal output
    terminalState.Update();
}

/// Render all subsystems
void WorkspaceRuntime::RenderFrame() {
    if (!canvasIntegration) return;
    
    canvasIntegration->BeginFrame();
    
    // Get main render target
    auto rt = canvasIntegration->GetRenderTarget();
    if (!rt) return;
    
    // Draw each panel based on layout
    RenderEditorPanel(rt);
    RenderChatPanel(rt);
    RenderFilePanel(rt);
    RenderTerminalPanel(rt);
    RenderCanvasPanel(rt);
    RenderTimelinePanel(rt);
    
    canvasIntegration->EndFrame();
}

void WorkspaceRuntime::RenderEditorPanel(ID2D1RenderTarget* rt) {
    Rect bounds = {0, 0, (float)canvasIntegration->GetViewportWidth() - 320, (float)canvasIntegration->GetViewportHeight() - 200};
    canvasIntegration->GetEditorRenderer()->Render(rt, bounds);
}

void WorkspaceRuntime::RenderChatPanel(ID2D1RenderTarget* rt) {
    float x = canvasIntegration->GetViewportWidth() - 320;
    float y = 0;
    float w = 320;
    float h = (canvasIntegration->GetViewportHeight() - 200) / 2;
    
    Rect bounds = {x, y, w, h};
    RenderChat(rt, bounds);
}

void WorkspaceRuntime::RenderFilePanel(ID2D1RenderTarget* rt) {
    float x = 0;
    float y = canvasIntegration->GetViewportHeight() - 200;
    float w = (canvasIntegration->GetViewportWidth() - 320) / 2;
    float h = 200;
    
    Rect bounds = {x, y, w, h};
    RenderFileTree(rt, bounds);
}

void WorkspaceRuntime::RenderTerminalPanel(ID2D1RenderTarget* rt) {
    float x = (canvasIntegration->GetViewportWidth() - 320) / 2;
    float y = canvasIntegration->GetViewportHeight() - 200;
    float w = (canvasIntegration->GetViewportWidth() - 320) / 2;
    float h = 200;
    
    Rect bounds = {x, y, w, h};
    RenderTerminal(rt, bounds);
}

void WorkspaceRuntime::RenderCanvasPanel(ID2D1RenderTarget* rt) {
    float x = canvasIntegration->GetViewportWidth() - 320;
    float y = (canvasIntegration->GetViewportHeight() - 200) / 2;
    float w = 320;
    float h = (canvasIntegration->GetViewportHeight() - 200) / 2;
    
    Rect bounds = {x, y, w, h};
    RenderCanvas(rt, bounds);
}

void WorkspaceRuntime::RenderTimelinePanel(ID2D1RenderTarget* rt) {
    float y = canvasIntegration->GetViewportHeight() - 200;
    float w = canvasIntegration->GetViewportWidth();
    float h = 200;
    
    Rect bounds = {0, y, w, h};
    RenderTimeline(rt, bounds);
}

// ============================================================================
// INPUT ROUTING
// ============================================================================

void WorkspaceRuntime::OnKeyPress(int vkey) {
    // Route to active panel
    
    // Editor shortcuts
    if ((GetKeyState(VK_CONTROL) & 0x8000)) {
        switch (vkey) {
            case 'Z':
                editorState.undo();
                break;
            case 'Y':
                editorState.redo();
                break;
            case 'K':
                // Ctrl+K → Chat focus
                activePanel = Panel::Chat;
                break;
            case 'F':
                // Ctrl+F → File search
                fileSystemState.SearchFiles("");
                break;
        }
        return;
    }
    
    // Route by active panel
    switch (activePanel) {
        case Panel::Editor:
            canvasIntegration->OnKeyPress(vkey);
            break;
        case Panel::Chat:
            OnChatKeyPress(vkey);
            break;
        case Panel::Files:
            OnFileKeyPress(vkey);
            break;
        case Panel::Terminal:
            OnTerminalKeyPress(vkey);
            break;
        default:
            break;
    }
}

void WorkspaceRuntime::OnMouseClick(int x, int y, int button) {
    // Determine which panel was clicked
    activePanel = GetPanelAtPoint(x, y);
    
    switch (activePanel) {
        case Panel::Editor:
            canvasIntegration->OnMouseClick(x, y, button);
            break;
        case Panel::Chat:
            OnChatMouseClick(x, y, button);
            break;
        case Panel::Files:
            OnFileMouseClick(x, y, button);
            break;
        case Panel::Terminal:
            OnTerminalMouseClick(x, y, button);
            break;
        default:
            break;
    }
}

void WorkspaceRuntime::OnMouseWheel(int x, int y, int delta) {
    auto panel = GetPanelAtPoint(x, y);
    
    if (panel == Panel::Editor) {
        canvasIntegration->OnMouseWheel(x, y, delta);
    } else if (panel == Panel::Chat) {
        OnChatMouseWheel(x, y, delta);
    }
}

void WorkspaceRuntime::OnChar(wchar_t c) {
    if (activePanel == Panel::Editor) {
        canvasIntegration->OnChar(c);
    } else if (activePanel == Panel::Chat) {
        OnChatChar(c);
    }
}

WorkspaceRuntime::Panel WorkspaceRuntime::GetPanelAtPoint(int x, int y) {
    float vw = (float)canvasIntegration->GetViewportWidth();
    float vh = (float)canvasIntegration->GetViewportHeight();
    
    // Right side chat
    if (x >= vw - 320) {
        if (y < (vh - 200) / 2) return Panel::Chat;
        else return Panel::Canvas;
    }
    
    // Bottom panels
    if (y >= vh - 200) {
        if (x < (vw - 320) / 2) return Panel::Files;
        else return Panel::Terminal;
    }
    
    // Center editor
    return Panel::Editor;
}

// ============================================================================
// AGENT INTEGRATION
// ============================================================================

void WorkspaceRuntime::LoadAgentConfig() {
    // Load from agents.json
    agentState.agents.push_back({
        "agent-1",
        "code-analyzer",
        {"analyze", "review", "refactor"},
        0.0
    });
    
    agentState.agents.push_back({
        "agent-2",
        "math-reasoner",
        {"prove", "simplify", "solve"},
        0.0
    });
}

void WorkspaceRuntime::OnUserPrompt(const std::string& prompt) {
    // Add to chat
    chatState.messages.push_back({
        "user",
        prompt
    });
    
    // Route to appropriate agent
    std::string response = DispatchAgent(prompt);
    
    chatState.messages.push_back({
        "agent",
        response
    });
    
    // Parse response and insert into editor if it's code
    if (response.find("```") != std::string::npos) {
        std::string code = ExtractCodeBlock(response);
        editorState.InsertString(code);
    }
}

std::string WorkspaceRuntime::DispatchAgent(const std::string& prompt) {
    // Simple agent routing based on keywords
    
    if (prompt.find("code") != std::string::npos ||
        prompt.find("refactor") != std::string::npos) {
        // Route to code-analyzer
        return CallAgent("code-analyzer", prompt);
    }
    
    if (prompt.find("math") != std::string::npos ||
        prompt.find("solve") != std::string::npos) {
        // Route to math-reasoner
        return CallAgent("math-reasoner", prompt);
    }
    
    // Default to MM-1 (generic reasoner)
    return CallAgent("default", prompt);
}

std::string WorkspaceRuntime::CallAgent(const std::string& agentName, const std::string& prompt) {
    // HTTP call to S7-MINI on port 5775
    // For now, return mock response
    return "```\n// Generated code\nint main() { return 0; }\n```";
}

std::string WorkspaceRuntime::ExtractCodeBlock(const std::string& text) {
    size_t start = text.find("```");
    if (start == std::string::npos) return "";
    
    start = text.find('\n', start) + 1;
    size_t end = text.find("```", start);
    
    return text.substr(start, end - start);
}

// ============================================================================
// TIMELINE STATE CAPTURE
// ============================================================================

void WorkspaceRuntime::CaptureTimelineFrame() {
    if (timelineState.frames.size() >= 100) {
        timelineState.frames.erase(timelineState.frames.begin());
    }
    
    WorkspaceFrame frame;
    frame.timestamp = std::chrono::system_clock::now();
    
    // Serialize all state
    json state;
    state["editor"] = json::object({
        {"lines", editorState.lines},
        {"cursorX", editorState.GetCursorX()},
        {"cursorY", editorState.GetCursorY()}
    });
    
    state["chat"] = json::array();
    for (const auto& msg : chatState.messages) {
        state["chat"].push_back({
            {"role", msg.role},
            {"text", msg.text}
        });
    }
    
    state["selectedFile"] = fileSystemState.selectedFile;
    
    frame.state = state;
    timelineState.frames.push_back(frame);
}

void WorkspaceRuntime::RestoreTimelineFrame(int frameIndex) {
    if (frameIndex < 0 || frameIndex >= timelineState.frames.size()) return;
    
    const auto& frame = timelineState.frames[frameIndex];
    
    // Restore editor
    editorState.Clear();
    auto lines = frame.state["editor"]["lines"];
    for (const auto& line : lines) {
        editorState.InsertString((std::string)line + "\n");
    }
    
    // Restore chat
    chatState.messages.clear();
    auto messages = frame.state["chat"];
    for (const auto& msg : messages) {
        chatState.messages.push_back({
            (std::string)msg["role"],
            (std::string)msg["text"]
        });
    }
}

// ============================================================================
// RENDERING HELPERS
// ============================================================================

void WorkspaceRuntime::RenderChat(ID2D1RenderTarget* rt, const Rect& bounds) {
    // Draw chat messages
    auto brush = canvasIntegration->GetResourceManager()->GetBrush(L"text");
    if (!brush) return;
    
    float y = bounds.y + 10;
    for (const auto& msg : chatState.messages) {
        std::wstring wmsg(msg.text.begin(), msg.text.end());
        rt->DrawTextW(
            wmsg.c_str(),
            wmsg.length(),
            canvasIntegration->GetResourceManager()->GetTextFormat(),
            D2D1::RectF(bounds.x + 5, y, bounds.x + bounds.w - 5, y + 40),
            brush
        );
        y += 45;
    }
}

void WorkspaceRuntime::RenderFileTree(ID2D1RenderTarget* rt, const Rect& bounds) {
    auto brush = canvasIntegration->GetResourceManager()->GetBrush(L"text");
    if (!brush) return;
    
    float y = bounds.y + 10;
    for (const auto& file : fileSystemState.files) {
        std::wstring wfile(file.begin(), file.end());
        rt->DrawTextW(
            wfile.c_str(),
            wfile.length(),
            canvasIntegration->GetResourceManager()->GetTextFormat(),
            D2D1::RectF(bounds.x + 5, y, bounds.x + bounds.w - 5, y + 18),
            brush
        );
        y += 20;
    }
}

void WorkspaceRuntime::RenderTerminal(ID2D1RenderTarget* rt, const Rect& bounds) {
    // Render terminal output
    auto brush = canvasIntegration->GetResourceManager()->GetBrush(L"text");
    if (!brush) return;
    
    float y = bounds.y + 10;
    for (const auto& line : terminalState.output) {
        std::wstring wline(line.begin(), line.end());
        rt->DrawTextW(
            wline.c_str(),
            wline.length(),
            canvasIntegration->GetResourceManager()->GetTextFormat(),
            D2D1::RectF(bounds.x + 5, y, bounds.x + bounds.w - 5, y + 18),
            brush
        );
        y += 18;
        if (y > bounds.y + bounds.h) break;
    }
}

void WorkspaceRuntime::RenderCanvas(ID2D1RenderTarget* rt, const Rect& bounds) {
    // Render canvas preview
    auto brush = canvasIntegration->GetResourceManager()->GetBrush(L"text");
    if (!brush) return;
    
    std::wstring preview = L"Canvas Preview (Task 8)";
    rt->DrawTextW(
        preview.c_str(),
        preview.length(),
        canvasIntegration->GetResourceManager()->GetTextFormat(),
        D2D1::RectF(bounds.x + 5, bounds.y + 5, bounds.x + bounds.w, bounds.y + 25),
        brush
    );
}

void WorkspaceRuntime::RenderTimeline(ID2D1RenderTarget* rt, const Rect& bounds) {
    auto brush = canvasIntegration->GetResourceManager()->GetBrush(L"linenum");
    if (!brush) return;
    
    // Draw timeline scrubber
    rt->FillRectangle(
        D2D1::RectF(bounds.x + 10, bounds.y + 10, bounds.x + 200, bounds.y + 20),
        brush
    );
}

// ============================================================================
// CLEANUP
// ============================================================================

void WorkspaceRuntime::Shutdown() {
    isRunning = false;
    if (canvasIntegration) {
        canvasIntegration->Shutdown();
        canvasIntegration.reset();
    }
}
