/// workspace_render_loop.cpp
/// Main render loop implementation

#include "workspace_render_loop.hpp"
#include <chrono>
#include <algorithm>

RenderLoopCoordinator* g_renderLoop = nullptr;

// ============================================================================
// RENDER LOOP COORDINATOR
// ============================================================================

RenderLoopCoordinator::RenderLoopCoordinator() {}

RenderLoopCoordinator::~RenderLoopCoordinator() {
    Shutdown();
}

bool RenderLoopCoordinator::Initialize(HWND hwnd, int width, int height) {
    renderCtx_.hwnd = hwnd;
    renderCtx_.width = width;
    renderCtx_.height = height;
    renderCtx_.hdc = GetDC(hwnd);

    workspace_.Initialize(width, height);
    renderer_.Initialize(hwnd);

    lastFrameTime_ = GetTickCount();

    return true;
}

void RenderLoopCoordinator::Shutdown() {
    workspace_.Shutdown();
    renderer_.Shutdown();

    if (renderCtx_.hdc) {
        ReleaseDC(renderCtx_.hwnd, renderCtx_.hdc);
        renderCtx_.hdc = NULL;
    }
}

void RenderLoopCoordinator::BeginFrame() {
    DWORD now = GetTickCount();
    renderCtx_.deltaTime = (now - lastFrameTime_) / 1000.0f;
    renderCtx_.tickCount = now;
    lastFrameTime_ = now;

    // Cap deltaTime at 100ms to prevent huge jumps
    if (renderCtx_.deltaTime > 0.1f) {
        renderCtx_.deltaTime = 0.1f;
    }
}

void RenderLoopCoordinator::Update(float deltaTime) {
    workspace_.Update(deltaTime);
    
    totalTime_ += deltaTime;
    frameCount_++;
}

void RenderLoopCoordinator::Render() {
    renderer_.BeginDraw();
    renderer_.Clear(0xFFF5F5F5);  // Light gray background

    // Layout: 4 panel arrangement
    // LEFT (240px): Files
    // CENTER (remaining): Editor (top) + Canvas (bottom)
    // RIGHT (320px): Chat
    // BOTTOM (200px): Terminal

    int filesWidth = 240;
    int chatWidth = 320;
    int terminalHeight = 200;

    // Draw panels (MVP: D2D/GDI rendering deferred to Phase 7.20)
    // For now, just clear and setup layout structures
    
    Rect filesRect = {0, 0, (float)filesWidth, (float)(renderCtx_.height - terminalHeight)};
    Rect editorRect = {(float)filesWidth, 0, (float)(renderCtx_.width - filesWidth - chatWidth), (float)(renderCtx_.height - terminalHeight)};
    Rect chatRect = {(float)(renderCtx_.width - chatWidth), 0, (float)chatWidth, (float)(renderCtx_.height - terminalHeight)};
    Rect terminalRect = {0, (float)(renderCtx_.height - terminalHeight), (float)renderCtx_.width, (float)terminalHeight};

    renderer_.EndDraw();
}

void RenderLoopCoordinator::EndFrame() {
    // Frame timing debug output (optional)
    // std::cout << "Frame " << frameCount_ << " (" << (1.0f / renderCtx_.deltaTime) << " FPS)\n";
}

void RenderLoopCoordinator::OnKeyDown(int vkey) {
    workspace_.OnKeyPress(vkey);
}

void RenderLoopCoordinator::OnKeyUp(int vkey) {
    workspace_.OnKeyRelease(vkey);
}

void RenderLoopCoordinator::OnChar(wchar_t c) {
    workspace_.OnChar(c);
}

void RenderLoopCoordinator::OnMouseDown(int x, int y, int button) {
    workspace_.OnMouseClick(x, y, button);
}

void RenderLoopCoordinator::OnMouseUp(int x, int y, int button) {
    // Handle mouse up (drag end, etc.)
}

void RenderLoopCoordinator::OnMouseMove(int x, int y) {
    workspace_.OnMouseMove(x, y);
}

void RenderLoopCoordinator::OnMouseWheel(int x, int y, int delta) {
    workspace_.OnMouseWheel(x, y, delta);
}

WorkspaceRuntime* RenderLoopCoordinator::GetRuntime() {
    return &workspace_;
}

CanvasRenderer* RenderLoopCoordinator::GetRenderer() {
    return &renderer_;
}

void RenderLoopCoordinator::RenderEditorPanel() {
    // Editor rendering (text, cursor)
}

void RenderLoopCoordinator::RenderChatPanel() {
    // Chat rendering (messages)
}

void RenderLoopCoordinator::RenderFilePanel() {
    // File tree rendering
}

void RenderLoopCoordinator::RenderTerminalPanel() {
    // Terminal output rendering
}

void RenderLoopCoordinator::RenderCanvasPanel() {
    // Canvas D2D rendering
}

void RenderLoopCoordinator::RenderDebugInfo() {
    // FPS counter, memory usage, etc.
}

Rect RenderLoopCoordinator::GetPanelRect(int panelIndex) {
    // Return rectangle for panel
    return Rect{0, 0, 100, 100};
}

// ============================================================================
// GLOBAL RENDER LOOP FUNCTIONS
// ============================================================================

void InitializeRenderLoop(HWND hwnd, int width, int height) {
    if (!g_renderLoop) {
        g_renderLoop = new RenderLoopCoordinator();
        g_renderLoop->Initialize(hwnd, width, height);
    }
}

void ShutdownRenderLoop() {
    if (g_renderLoop) {
        g_renderLoop->Shutdown();
        delete g_renderLoop;
        g_renderLoop = nullptr;
    }
}

void MainLoopTick() {
    if (!g_renderLoop) return;

    g_renderLoop->BeginFrame();
    g_renderLoop->Update(0.016f);
    g_renderLoop->Render();
    g_renderLoop->EndFrame();
}
