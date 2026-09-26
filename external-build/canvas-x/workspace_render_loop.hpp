/// workspace_render_loop.hpp
/// Main event loop and unified rendering

#pragma once

#include "workspace_runtime.hpp"
#include "workspace_canvas_d2d.hpp"
#include <windows.h>

// ============================================================================
// RENDER CONTEXT
// ============================================================================

struct RenderContext {
    HDC hdc = NULL;
    HWND hwnd = NULL;
    int width = 0;
    int height = 0;
    float deltaTime = 0.016f;  // ~60 FPS
    DWORD tickCount = 0;
};

// ============================================================================
// MAIN RENDER LOOP COORDINATOR
// ============================================================================

class RenderLoopCoordinator {
public:
    RenderLoopCoordinator();
    ~RenderLoopCoordinator();

    // Lifecycle
    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();

    // Main loop
    void BeginFrame();
    void Update(float deltaTime);
    void Render();
    void EndFrame();

    // Event routing
    void OnKeyDown(int vkey);
    void OnKeyUp(int vkey);
    void OnChar(wchar_t c);
    void OnMouseDown(int x, int y, int button);
    void OnMouseUp(int x, int y, int button);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int x, int y, int delta);

    // State access
    WorkspaceRuntime* GetRuntime();
    CanvasRenderer* GetRenderer();

private:
    WorkspaceRuntime workspace_;
    CanvasRenderer renderer_;
    RenderContext renderCtx_;
    DWORD lastFrameTime_ = 0;
    int frameCount_ = 0;
    float totalTime_ = 0.0f;

    void RenderEditorPanel();
    void RenderChatPanel();
    void RenderFilePanel();
    void RenderTerminalPanel();
    void RenderCanvasPanel();
    void RenderDebugInfo();

    Rect GetPanelRect(int panelIndex);
};

// ============================================================================
// GLOBAL RENDER LOOP STATE
// ============================================================================

extern RenderLoopCoordinator* g_renderLoop;

void InitializeRenderLoop(HWND hwnd, int width, int height);
void ShutdownRenderLoop();
void MainLoopTick();
