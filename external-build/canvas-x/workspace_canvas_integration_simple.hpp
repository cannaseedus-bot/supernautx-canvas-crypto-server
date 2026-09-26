#pragma once

#include "workspace_editor_d2d_minimal.hpp"
#include <d2d1.h>
#include <dwrite.h>

class D2DResourceManager {
public:
    bool Initialize(void* hwnd) { return true; }
    void Cleanup() {}
};

class EditorRenderDelegate {
public:
    EditorRenderDelegate(D2DEditor* editor, D2DResourceManager* resources) {}
    void Render(void* rt, const Rect& bounds) {}
};

class WorkspaceCanvasIntegration {
public:
    WorkspaceCanvasIntegration(void* runtime) {}
    bool Initialize(void* hwnd) { return true; }
    void Render() {}
    void HandleInput(int x, int y) {}
};
