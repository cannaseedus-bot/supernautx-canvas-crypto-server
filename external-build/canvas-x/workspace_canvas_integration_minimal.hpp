/// workspace_canvas_integration_minimal.hpp
/// Minimal Canvas integration

#pragma once

#include "workspace_editor_d2d_minimal.hpp"
#include <d2d1.h>
#include <dwrite.h>
#include <windows.h>

class WorkspaceRuntime;

class WorkspaceCanvasIntegration {
public:
    WorkspaceCanvasIntegration(WorkspaceRuntime* runtime) : runtime(runtime) {}
    
    bool Initialize(HWND hwnd, int width, int height) {
        return true;
    }
    
    void Shutdown() {}
    void BeginFrame() {}
    void Render() {}
    void EndFrame() {}
    
    void OnKeyPress(int vkey) {}
    void OnKeyRelease(int vkey) {}
    void OnChar(wchar_t c) {}
    void OnMouseClick(int x, int y, int btn) {}
    void OnMouseMove(int x, int y) {}
    void OnMouseWheel(int x, int y, int delta) {}
    
    ID2D1RenderTarget* GetRenderTarget() { return nullptr; }
    int GetViewportWidth() { return 1024; }
    int GetViewportHeight() { return 768; }
    
private:
    D2DEditor editor;
    WorkspaceRuntime* runtime;
};

#endif
