/// workspace_editor_test.cpp
/// Phase 7.19 Editor Test — Single-Window D2D Editor Verification

#include "workspace_canvas_integration_minimal.hpp"
#include <windows.h>
#include <iostream>

WorkspaceCanvasIntegration* g_editor = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT:
            if (g_editor) {
                g_editor->BeginFrame();
                g_editor->Render();
                g_editor->EndFrame();
            }
            ValidateRect(hwnd, NULL);
            return 0;
            
        case WM_KEYDOWN:
            if (g_editor) {
                g_editor->OnKeyPress((int)wParam);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_KEYUP:
            if (g_editor) {
                g_editor->OnKeyRelease((int)wParam);
            }
            return 0;
            
        case WM_CHAR:
            if (g_editor) {
                g_editor->OnChar((wchar_t)wParam);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_LBUTTONDOWN:
            if (g_editor) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                g_editor->OnMouseClick(x, y, 1);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_MOUSEMOVE:
            if (g_editor) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                g_editor->OnMouseMove(x, y);
            }
            return 0;
            
        case WM_MOUSEWHEEL:
            if (g_editor) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                g_editor->OnMouseWheel(x, y, delta);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    const wchar_t CLASS_NAME[] = L"Phase7.19EditorTestClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    
    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Phase 7.19: Workspace Editor Test",
        WS_OVERLAPPEDWINDOW,
        
        CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 768,
        
        NULL,
        NULL,
        NULL,
        NULL
    );
    
    if (hwnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }
    
    // Initialize editor
    g_editor = new WorkspaceCanvasIntegration(nullptr);
    if (!g_editor->Initialize(hwnd, 1024, 768)) {
        std::cerr << "Failed to initialize editor" << std::endl;
        return 1;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    
    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        // Render every frame
        if (g_editor) {
            g_editor->BeginFrame();
            g_editor->Render();
            g_editor->EndFrame();
        }
    }
    
    // Cleanup
    if (g_editor) {
        g_editor->Shutdown();
        delete g_editor;
    }
    
    return 0;
}
