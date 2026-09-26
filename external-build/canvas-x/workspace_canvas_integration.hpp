/// workspace_canvas_integration.hpp
/// Bridge between D2D Editor and Canvas.X Rendering System
/// Handles D2D context setup, rendering dispatch, and input routing

#pragma once

#include "workspace_editor_d2d_minimal.hpp"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl.h>
#include <map>

// Forward declaration to avoid circular includes
class WorkspaceRuntime;

// ============================================================================
// D2D RESOURCE MANAGER
// ============================================================================

class D2DResourceManager {
public:
    bool Initialize(HWND hwnd);
    void Cleanup();
    
    ID2D1Factory* GetFactory() const { return factory.Get(); }
    ID2D1RenderTarget* GetRenderTarget() const { return renderTarget.Get(); }
    IDWriteFactory* GetWriteFactory() const { return writeFactory.Get(); }
    IDWriteTextFormat* GetTextFormat() const { return textFormat.Get(); }
    
    void CreateBrush(const wchar_t* name, D2D1_COLOR_F color, ID2D1Brush** brush);
    ID2D1Brush* GetBrush(const wchar_t* name);
    
private:
    ComPtr<ID2D1Factory> factory;
    ComPtr<ID2D1RenderTarget> renderTarget;
    ComPtr<IDWriteFactory> writeFactory;
    ComPtr<IDWriteTextFormat> textFormat;
    std::map<std::wstring, ComPtr<ID2D1SolidColorBrush>> brushes;
};

// ============================================================================
// EDITOR RENDER DELEGATE
// ============================================================================

class EditorRenderDelegate {
public:
    EditorRenderDelegate(D2DEditor* editor, D2DResourceManager* resources);
    
    void Render(ID2D1RenderTarget* rt, const Rect& bounds);
    
private:
    D2DEditor* editor;
    D2DResourceManager* resources;
    
    void RenderLines(ID2D1RenderTarget* rt, const Rect& bounds);
    void RenderLineNumbers(ID2D1RenderTarget* rt, const Rect& bounds);
    void RenderCursor(ID2D1RenderTarget* rt, const Rect& bounds);
    void RenderSelection(ID2D1RenderTarget* rt, const Rect& bounds);
    void RenderScrollBar(ID2D1RenderTarget* rt, const Rect& bounds);
    void RenderLineBackground(ID2D1RenderTarget* rt, const Rect& bounds, int lineNum, float y);
};

// ============================================================================
// WORKSPACE CANVAS INTEGRATION
// ============================================================================

class WorkspaceCanvasIntegration {
public:
    WorkspaceCanvasIntegration(WorkspaceRuntime* runtime);
    
    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();
    
    // Rendering
    void BeginFrame();
    void Render();
    void EndFrame();
    
    // Input routing
    void OnKeyPress(int vkey);
    void OnKeyRelease(int vkey);
    void OnMouseClick(int x, int y, int button);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int x, int y, int delta);
    void OnChar(wchar_t c);
    
    // State
    bool IsInitialized() const { return isInitialized; }
    D2DEditor* GetEditor() { return &editor; }
    
private:
    WorkspaceRuntime* runtime;
    D2DEditor editor;
    D2DResourceManager resourceMgr;
    EditorRenderDelegate editorRenderer;
    
    int viewportWidth = 0;
    int viewportHeight = 0;
    bool isInitialized = false;
    
    // Input state
    bool isCtrlPressed = false;
    bool isShiftPressed = false;
    bool isAltPressed = false;
};

// ============================================================================
// HELPER MACROS
// ============================================================================

#define HR(x) { HRESULT hr = (x); if (FAILED(hr)) return false; }

#endif // WORKSPACE_CANVAS_INTEGRATION_HPP
