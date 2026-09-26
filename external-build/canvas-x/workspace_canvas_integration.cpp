/// workspace_canvas_integration.cpp
/// Canvas.X Bridge Implementation - D2D Context Setup & Rendering

#include "workspace_canvas_integration.hpp"
#include <wrl.h>

using Microsoft::WRL::ComPtr;

// ============================================================================
// D2D RESOURCE MANAGER IMPLEMENTATION
// ============================================================================

bool D2DResourceManager::Initialize(HWND hwnd) {
    if (!hwnd) return false;
    
    // Create D2D factory
    HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.ReleaseAndGetAddressOf()));
    
    // Get window client rect
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    D2D1_SIZE_U size = D2D1::SizeU(
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    
    // Create render target
    HR(factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        (ID2D1HwndRenderTarget**)renderTarget.ReleaseAndGetAddressOf()
    ));
    
    // Create DirectWrite factory
    HR(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        (IUnknown**)writeFactory.ReleaseAndGetAddressOf()
    ));
    
    // Create text format
    HR(writeFactory->CreateTextFormat(
        L"Courier New",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-us",
        textFormat.ReleaseAndGetAddressOf()
    ));
    
    // Set text format properties
    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    
    // Create default brushes
    ComPtr<ID2D1SolidColorBrush> textBrush;
    HR(renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        textBrush.ReleaseAndGetAddressOf()
    ));
    brushes[L"text"] = textBrush;
    
    ComPtr<ID2D1SolidColorBrush> cursorBrush;
    HR(renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::LimeGreen),
        cursorBrush.ReleaseAndGetAddressOf()
    ));
    brushes[L"cursor"] = cursorBrush;
    
    ComPtr<ID2D1SolidColorBrush> selectionBrush;
    HR(renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0x4A6CF7, 0.3f),
        selectionBrush.ReleaseAndGetAddressOf()
    ));
    brushes[L"selection"] = selectionBrush;
    
    ComPtr<ID2D1SolidColorBrush> lineNumBrush;
    HR(renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Gray),
        lineNumBrush.ReleaseAndGetAddressOf()
    ));
    brushes[L"linenum"] = lineNumBrush;
    
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    HR(renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0x1E1E1E),
        bgBrush.ReleaseAndGetAddressOf()
    ));
    brushes[L"background"] = bgBrush;
    
    return true;
}

void D2DResourceManager::Cleanup() {
    brushes.clear();
    textFormat.Reset();
    writeFactory.Reset();
    renderTarget.Reset();
    factory.Reset();
}

void D2DResourceManager::CreateBrush(const wchar_t* name, D2D1_COLOR_F color, ID2D1Brush** brush) {
    if (!renderTarget) return;
    
    ComPtr<ID2D1SolidColorBrush> solidBrush;
    if (SUCCEEDED(renderTarget->CreateSolidColorBrush(color, solidBrush.ReleaseAndGetAddressOf()))) {
        brushes[name] = solidBrush;
        *brush = solidBrush.Get();
    }
}

ID2D1Brush* D2DResourceManager::GetBrush(const wchar_t* name) {
    auto it = brushes.find(name);
    if (it != brushes.end()) {
        return it->second.Get();
    }
    return nullptr;
}

// ============================================================================
// EDITOR RENDER DELEGATE IMPLEMENTATION
// ============================================================================

EditorRenderDelegate::EditorRenderDelegate(D2DEditor* editor, D2DResourceManager* resources)
    : editor(editor), resources(resources) {
}

void EditorRenderDelegate::Render(ID2D1RenderTarget* rt, const Rect& bounds) {
    if (!editor || !resources || !rt) return;
    
    // Draw background
    auto bgBrush = resources->GetBrush(L"background");
    if (bgBrush) {
        rt->FillRectangle(
            D2D1::RectF(bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h),
            bgBrush
        );
    }
    
    // Render components
    RenderLineNumbers(rt, bounds);
    RenderLines(rt, bounds);
    RenderSelection(rt, bounds);
    RenderCursor(rt, bounds);
    RenderScrollBar(rt, bounds);
}

void EditorRenderDelegate::RenderLines(ID2D1RenderTarget* rt, const Rect& bounds) {
    const auto& lines = editor->GetLines();
    auto textBrush = resources->GetBrush(L"text");
    auto textFormat = resources->GetWriteFactory();
    
    if (!textBrush || !textFormat) return;
    
    float y = bounds.y + 10.0f;
    int lineNum = 0;
    
    for (const auto& line : lines) {
        if (y > bounds.y + bounds.h) break;
        
        std::wstring wLine(line.begin(), line.end());
        
        rt->DrawTextW(
            wLine.c_str(),
            wLine.length(),
            resources->GetTextFormat(),
            D2D1::RectF(bounds.x + 60.0f, y, bounds.x + bounds.w, y + 18.0f),
            textBrush
        );
        
        y += 18.0f;
        lineNum++;
    }
}

void EditorRenderDelegate::RenderLineNumbers(ID2D1RenderTarget* rt, const Rect& bounds) {
    const auto& lines = editor->GetLines();
    auto lineNumBrush = resources->GetBrush(L"linenum");
    
    if (!lineNumBrush) return;
    
    float y = bounds.y + 10.0f;
    int lineNum = 1;
    
    for (int i = 0; i < lines.size(); ++i) {
        if (y > bounds.y + bounds.h) break;
        
        wchar_t lineNumStr[16];
        wsprintf(lineNumStr, L"%d", lineNum);
        
        rt->DrawTextW(
            lineNumStr,
            wcslen(lineNumStr),
            resources->GetTextFormat(),
            D2D1::RectF(bounds.x + 5.0f, y, bounds.x + 50.0f, y + 18.0f),
            lineNumBrush
        );
        
        y += 18.0f;
        lineNum++;
    }
}

void EditorRenderDelegate::RenderCursor(ID2D1RenderTarget* rt, const Rect& bounds) {
    const auto& cursor = editor->GetCursor();
    
    if (!cursor.IsVisible()) return;
    
    auto cursorBrush = resources->GetBrush(L"cursor");
    if (!cursorBrush) return;
    
    float x = bounds.x + 60.0f + cursor.x * 10.0f;
    float y = bounds.y + 10.0f + cursor.y * 18.0f;
    
    rt->FillRectangle(
        D2D1::RectF(x, y, x + 2.0f, y + 18.0f),
        cursorBrush
    );
}

void EditorRenderDelegate::RenderSelection(ID2D1RenderTarget* rt, const Rect& bounds) {
    auto selectionBrush = resources->GetBrush(L"selection");
    if (!selectionBrush) return;
    
    // Render selection highlighting (simplified)
    // Full implementation would iterate through selected lines
}

void EditorRenderDelegate::RenderScrollBar(ID2D1RenderTarget* rt, const Rect& bounds) {
    auto scrollPy = editor->GetScrollPercentY();
    float scrollHeight = bounds.h * 0.2f;  // Arbitrary
    float scrollY = bounds.y + (bounds.h - scrollHeight) * scrollPy;
    
    auto scrollBrush = resources->GetBrush(L"linenum");
    if (!scrollBrush) return;
    
    rt->FillRectangle(
        D2D1::RectF(
            bounds.x + bounds.w - 10.0f,
            scrollY,
            bounds.x + bounds.w - 2.0f,
            scrollY + scrollHeight
        ),
        scrollBrush
    );
}

// ============================================================================
// WORKSPACE CANVAS INTEGRATION IMPLEMENTATION
// ============================================================================

WorkspaceCanvasIntegration::WorkspaceCanvasIntegration(WorkspaceRuntime* runtime)
    : runtime(runtime), editorRenderer(&editor, &resourceMgr) {
}

bool WorkspaceCanvasIntegration::Initialize(HWND hwnd, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    
    // Initialize D2D resources
    if (!resourceMgr.Initialize(hwnd)) {
        return false;
    }
    
    // Initialize editor
    EditorRenderContext ctx;
    ctx.renderTarget = resourceMgr.GetRenderTarget();
    ctx.writeFactory = resourceMgr.GetWriteFactory();
    ctx.textFormat = resourceMgr.GetTextFormat();
    ctx.textBrush = resourceMgr.GetBrush(L"text");
    ctx.cursorBrush = resourceMgr.GetBrush(L"cursor");
    ctx.lineNumBrush = resourceMgr.GetBrush(L"linenum");
    ctx.selectionBrush = resourceMgr.GetBrush(L"selection");
    
    editor.Initialize(ctx, width - 60, height);
    
    // Load some sample code
    editor.SetContent(
        "// Phase 7.19: Unified Workspace\n"
        "// D2D Code Editor with Chat Integration\n"
        "\n"
        "int main() {\n"
        "    // Start typing here...\n"
        "    return 0;\n"
        "}\n"
    );
    
    isInitialized = true;
    return true;
}

void WorkspaceCanvasIntegration::Shutdown() {
    isInitialized = false;
    resourceMgr.Cleanup();
}

void WorkspaceCanvasIntegration::BeginFrame() {
    if (!resourceMgr.GetRenderTarget()) return;
    resourceMgr.GetRenderTarget()->BeginDraw();
}

void WorkspaceCanvasIntegration::Render() {
    if (!isInitialized) return;
    
    Rect editorBounds = {60.0f, 10.0f, (float)viewportWidth - 70.0f, (float)viewportHeight};
    editorRenderer.Render(resourceMgr.GetRenderTarget(), editorBounds);
}

void WorkspaceCanvasIntegration::EndFrame() {
    if (!resourceMgr.GetRenderTarget()) return;
    
    HRESULT hr = resourceMgr.GetRenderTarget()->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        resourceMgr.Cleanup();
        // Recreate resources
    }
}

void WorkspaceCanvasIntegration::OnKeyPress(int vkey) {
    isCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    isShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    isAltPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
    
    // Handle keyboard shortcuts
    if (isCtrlPressed) {
        switch (vkey) {
            case 'Z':
                editor.Undo();
                break;
            case 'Y':
                editor.Redo();
                break;
            case 'A':
                editor.SelectAll();
                break;
            case 'C':
                {
                    std::string selected = editor.GetSelectedText();
                    // Copy to clipboard
                }
                break;
            case 'V':
                {
                    // Paste from clipboard
                }
                break;
        }
        return;
    }
    
    // Navigation
    switch (vkey) {
        case VK_LEFT:
            editor.MoveCursorLeft();
            break;
        case VK_RIGHT:
            editor.MoveCursorRight();
            break;
        case VK_UP:
            editor.MoveCursorUp();
            break;
        case VK_DOWN:
            editor.MoveCursorDown();
            break;
        case VK_HOME:
            editor.MoveCursorHome();
            break;
        case VK_END:
            editor.MoveCursorEnd();
            break;
        case VK_PRIOR:
            editor.MoveCursorPageUp();
            break;
        case VK_NEXT:
            editor.MoveCursorPageDown();
            break;
        case VK_DELETE:
            editor.Delete();
            break;
        case VK_BACK:
            editor.Backspace();
            break;
        case VK_RETURN:
            editor.NewLine();
            break;
        case VK_TAB:
            if (isShiftPressed) {
                editor.UnindentLine();
            } else {
                editor.IndentLine();
            }
            break;
    }
}

void WorkspaceCanvasIntegration::OnKeyRelease(int vkey) {
    // Handle key releases if needed
}

void WorkspaceCanvasIntegration::OnMouseClick(int x, int y, int button) {
    if (button == 1) {  // Left click
        editor.OnMouseClick(x, y);
    }
}

void WorkspaceCanvasIntegration::OnMouseMove(int x, int y) {
    // Update status bar with cursor position
}

void WorkspaceCanvasIntegration::OnMouseWheel(int x, int y, int delta) {
    editor.OnMouseWheel(delta);
}

void WorkspaceCanvasIntegration::OnChar(wchar_t c) {
    if (c >= 32 && c < 127) {
        editor.InsertChar((char)c);
        
        // Notify runtime for potential AI integration
        if (runtime && c == ' ') {
            // Could trigger auto-complete here
        }
    }
}
